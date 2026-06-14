// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPoint.generated.h"

/**
 *  Finds a random reachable point on the NavMesh within the controller's PatrolRadius
 *  and stores it in the Blackboard PatrolLocation key.
 */
UCLASS()
class NEWPROJECT_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** Blackboard key to store the patrol destination */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName PatrolLocationKey = TEXT("PatrolLocation");
};
