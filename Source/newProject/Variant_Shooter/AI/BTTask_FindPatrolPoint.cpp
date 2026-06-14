// Copyright Epic Games, Inc. All Rights Reserved.

#include "BTTask_FindPatrolPoint.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "NavigationSystem.h"
#include "ShooterAIController.h"

UBTTask_FindPatrolPoint::UBTTask_FindPatrolPoint()
{
	NodeName = TEXT("Find Patrol Point");
}

EBTNodeResult::Type UBTTask_FindPatrolPoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	AAIController* AICont = OwnerComp.GetAIOwner();
	APawn* OwnerPawn = AICont ? AICont->GetPawn() : nullptr;
	UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

	if (!OwnerPawn || !Blackboard)
	{
		return EBTNodeResult::Failed;
	}

	AShooterAIController* ShooterAI = Cast<AShooterAIController>(AICont);
	const float Radius = ShooterAI ? ShooterAI->GetPatrolRadius() : 1500.0f;
	const FVector Origin = OwnerPawn->GetActorLocation();

	FNavLocation NavPoint;
	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(OwnerPawn->GetWorld()))
	{
		if (NavSys->GetRandomReachablePointInRadius(Origin, Radius, NavPoint))
		{
			Blackboard->SetValueAsVector(PatrolLocationKey, NavPoint.Location);
			return EBTNodeResult::Succeeded;
		}
	}

	// fallback when NavMesh is not available or query fails
	FVector FallbackOffset = FMath::VRand() * FMath::RandRange(100.0f, Radius);
	FallbackOffset.Z = 0.0f;
	Blackboard->SetValueAsVector(PatrolLocationKey, Origin + FallbackOffset);
	return EBTNodeResult::Succeeded;
}
