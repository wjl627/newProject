// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTService_UpdatePerception.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AIController.h"
#include "ShooterAIController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

UBTService_UpdatePerception::UBTService_UpdatePerception()
{
	NodeName = TEXT("Update Perception");
	Interval = 0.2f;
	RandomDeviation = 0.05f;
}

void UBTService_UpdatePerception::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

	FBTUpdatePerceptionMemory* Memory = reinterpret_cast<FBTUpdatePerceptionMemory*>(NodeMemory);
	if (!Memory)
	{
		return;
	}

	AAIController* AICont = OwnerComp.GetAIOwner();
	UAIPerceptionComponent* Perception = AICont ? AICont->GetPerceptionComponent() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();
	APawn* OwnerPawn = AICont ? AICont->GetPawn() : nullptr;

	if (!Perception || !Blackboard || !OwnerPawn)
	{
		return;
	}

	TArray<AActor*> PerceivedActors;
	Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), PerceivedActors);

	AShooterAIController* ShooterAI = Cast<AShooterAIController>(AICont);
	const FName SenseTag = ShooterAI ? ShooterAI->GetSenseTag() : FName("Player");

	AActor* BestTarget = nullptr;
	float BestDistSq = MAX_FLT;

	// 始终检测任何非常靠近的物体，即使它在视线锥之外或 AI 身后。
	// 防止敌人在近距离内无视玩家。
	if (ShooterAI && ShooterAI->GetCloseRangeAutoDetectRadius() > 0.0f)
	{
		TArray<AActor*> CloseActors;
		UGameplayStatics::GetAllActorsWithTag(OwnerPawn->GetWorld(), SenseTag, CloseActors);
		const float CloseRangeSq = FMath::Square(ShooterAI->GetCloseRangeAutoDetectRadius());
		for (AActor* Actor : CloseActors)
		{
			if (Actor && !Actor->ActorHasTag(FName("Dead")))
			{
				const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), OwnerPawn->GetActorLocation());
				if (DistSq <= CloseRangeSq && DistSq < BestDistSq)
				{
					BestDistSq = DistSq;
					BestTarget = Actor;
				}
			}
		}
	}

	for (AActor* Actor : PerceivedActors)
	{
		if (Actor && Actor->ActorHasTag(SenseTag) && !Actor->ActorHasTag(FName("Dead")))
		{
			const float DistSq = FVector::DistSquared(Actor->GetActorLocation(), OwnerPawn->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BestTarget = Actor;
			}
		}
	}

	// 提前同步 TargetEnemy，以便下面的 HasLineOfSightToTarget 正常工作
	if (ShooterAI)
	{
		ShooterAI->SetCurrentTarget(BestTarget);
	}

	if (BestTarget)
	{
		Memory->LostTargetTime = 0.0f;

		// 记住目标位置，以便 AI 在失去视野后仍能追击
		ShooterAI->UpdateLastKnownTargetLocation(BestTarget->GetActorLocation());
		Blackboard->SetValueAsVector(PatrolLocationKey, BestTarget->GetActorLocation());

		const float Distance = FMath::Sqrt(BestDistSq);
		const bool bInAttackRange = Distance <= ShooterAI->GetAttackRange();
		const bool bHasLOS = ShooterAI && ShooterAI->HasLineOfSightToTarget();

		if (bInAttackRange && bHasLOS)
		{
			Memory->LostAttackTime = 0.0f;
		}
		else
		{
			Memory->LostAttackTime += DeltaSeconds;
		}

		// 如果最近正在攻击，即使超出 LoseTargetRange 也继续追击
		if (Distance > ShooterAI->GetLoseTargetRange())
		{
			if (Memory->LostAttackTime > FBTUpdatePerceptionMemory::ChasePersistTime)
			{
				BestTarget = nullptr;
			}
		}
	}
	else
	{
		Memory->LostTargetTime += DeltaSeconds;
		Memory->LostAttackTime += DeltaSeconds;

		const float EffectiveGrace = (Memory->LostAttackTime <= FBTUpdatePerceptionMemory::ChasePersistTime)
			? FBTUpdatePerceptionMemory::ChasePersistTime
			: FBTUpdatePerceptionMemory::GracePeriod;

		if (Memory->LostTargetTime <= EffectiveGrace)
		{
			BestTarget = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey));
			if (BestTarget && (!IsValid(BestTarget) || BestTarget->ActorHasTag(FName("Dead"))))
			{
				BestTarget = nullptr;
			}
			if (BestTarget)
			{
				BestDistSq = FVector::DistSquared(BestTarget->GetActorLocation(), OwnerPawn->GetActorLocation());
				if (ShooterAI)
				{
					ShooterAI->SetCurrentTarget(BestTarget);
					ShooterAI->UpdateLastKnownTargetLocation(BestTarget->GetActorLocation());
					Blackboard->SetValueAsVector(PatrolLocationKey, BestTarget->GetActorLocation());
				}
			}
		}
	}

	if (BestTarget)
	{
		const float Distance = FMath::Sqrt(BestDistSq);
		Blackboard->SetValueAsBool(bInAttackRangeKey, Distance <= ShooterAI->GetAttackRange());
		Blackboard->SetValueAsBool(bHasLineOfSightKey, ShooterAI && ShooterAI->HasLineOfSightToTarget());
	}
	else
	{
		Blackboard->ClearValue(bInAttackRangeKey);
		Blackboard->ClearValue(bHasLineOfSightKey);
	}

	Blackboard->SetValueAsObject(TargetActorKey, BestTarget);
}
