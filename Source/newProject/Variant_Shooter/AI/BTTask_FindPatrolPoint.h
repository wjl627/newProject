// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_FindPatrolPoint.generated.h"

/**
 *  在控制器 PatrolRadius 范围内的 NavMesh 上查找一个随机可达点，
 *  并将其存储到黑板（Blackboard）的 PatrolLocation 键中。
 */
UCLASS()
class NEWPROJECT_API UBTTask_FindPatrolPoint : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_FindPatrolPoint();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	/** 用于存储巡逻目的地的黑板键 */
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName PatrolLocationKey = TEXT("PatrolLocation");
};
