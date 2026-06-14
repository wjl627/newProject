// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdatePerception.generated.h"

struct FBTUpdatePerceptionMemory
{
	float LostTargetTime = 0.0f;
	float LostAttackTime = 0.0f;

	static constexpr float GracePeriod = 0.5f;
	static constexpr float ChasePersistTime = 3.0f;
};

UCLASS()
class NEWPROJECT_API UBTService_UpdatePerception : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdatePerception();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual uint16 GetInstanceMemorySize() const override { return sizeof(FBTUpdatePerceptionMemory); }

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName TargetActorKey = TEXT("TargetActor");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName bInAttackRangeKey = TEXT("bInAttackRange");

	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FName bHasLineOfSightKey = TEXT("bHasLineOfSight");
};
