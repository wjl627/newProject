// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTService_UpdatePerception.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISense_Sight.h"
#include "AIController.h"
#include "ShooterAIController.h"

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

	// Sync TargetEnemy early so HasLineOfSightToTarget works below
	if (ShooterAI)
	{
		ShooterAI->SetCurrentTarget(BestTarget);
	}

	if (BestTarget)
	{
		Memory->LostTargetTime = 0.0f;

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

		// Keep chasing beyond LoseTargetRange if we were recently attacking
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
