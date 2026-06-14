// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_FireWeapon.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "ShooterNPC.h"

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
