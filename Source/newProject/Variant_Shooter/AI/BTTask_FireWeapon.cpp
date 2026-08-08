// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_FireWeapon.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "ShooterNPC.h"
#include "ShooterAIController.h"

UBTTask_FireWeapon::UBTTask_FireWeapon()
{
	NodeName = TEXT("Fire Weapon");
}

EBTNodeResult::Type UBTTask_FireWeapon::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICont = OwnerComp.GetAIOwner();
	AShooterNPC* NPC = AICont ? Cast<AShooterNPC>(AICont->GetPawn()) : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!NPC || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AActor* Target = Cast<AActor>(Blackboard->GetValueAsObject(TargetActorKey));
	if (!Target)
	{
		return EBTNodeResult::Failed;
	}

	// 防御性检查：只有目标确实在射程内且可见时才开枪。
	// 否则返回失败，让行为树回退到追击/移动分支。
	if (AShooterAIController* ShooterAI = Cast<AShooterAIController>(AICont))
	{
		ShooterAI->SetCurrentTarget(Target);
		const float Distance = FVector::Dist(Target->GetActorLocation(), NPC->GetActorLocation());
		if (Distance > ShooterAI->GetAttackRange() || !ShooterAI->HasLineOfSightToTarget())
		{
			return EBTNodeResult::Failed;
		}
	}

	AICont->StopMovement();
	AICont->SetFocus(Target);
	NPC->StartShooting(Target);
	return EBTNodeResult::InProgress;
}

EBTNodeResult::Type UBTTask_FireWeapon::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICont = OwnerComp.GetAIOwner();
	AShooterNPC* NPC = AICont ? Cast<AShooterNPC>(AICont->GetPawn()) : nullptr;

	if (NPC)
	{
		NPC->StopShooting();
	}

	if (AICont)
	{
		AICont->ClearFocus(EAIFocusPriority::Gameplay);
	}

	return EBTNodeResult::Aborted;
}
