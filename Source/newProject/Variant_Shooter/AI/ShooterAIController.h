// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShooterAIController.generated.h"

class UAIPerceptionComponent;
class AShooterNPC;
class UBehaviorTree;
struct FAIStimulus;

DECLARE_DELEGATE_TwoParams(FShooterPerceptionUpdatedDelegate, AActor*, const FAIStimulus&);
DECLARE_DELEGATE_OneParam(FShooterPerceptionForgottenDelegate, AActor*);

// -----------------------------------------------------------------------------
// State Machine — deprecated, kept for reference
// -----------------------------------------------------------------------------
#if 0

UENUM()
enum class EAIState : uint8
{
	Patrol,
	Chase,
	Attack
};

#endif // State Machine

UCLASS(abstract)
class NEWPROJECT_API AShooterAIController : public AAIController
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionComponent* AIPerception;

protected:

	UPROPERTY(EditAnywhere, Category="Shooter")
	FName TeamTag = FName("Enemy");

	UPROPERTY(EditAnywhere, Category="Shooter|Perception")
	FName SenseTag = FName("Player");

	// --- Patrol ---

	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, Units = "cm"))
	float PatrolRadius = 1500.0f;

#if 0 // State Machine
	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float MinPatrolWait = 1.0f;

	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float MaxPatrolWait = 3.0f;
#endif

	// --- Combat ---

	UPROPERTY(EditAnywhere, Category="Shooter|Combat", meta = (ClampMin = 0, Units = "cm"))
	float AttackRange = 2000.0f;

	UPROPERTY(EditAnywhere, Category="Shooter|Combat", meta = (ClampMin = 0, Units = "cm"))
	float LoseTargetRange = 4000.0f;

	// --- Runtime ---

	TObjectPtr<AActor> TargetEnemy;

	TObjectPtr<AShooterNPC> PossessedNPC;

#if 0 // State Machine
	EAIState CurrentState = EAIState::Patrol;
	FTimerHandle PatrolWaitTimer;
	bool bIsWaitingAtPatrol = false;
#endif

public:

	AShooterAIController();

	FShooterPerceptionUpdatedDelegate OnShooterPerceptionUpdated;
	FShooterPerceptionForgottenDelegate OnShooterPerceptionForgotten;

	void SetCurrentTarget(AActor* Target) { TargetEnemy = Target; }
	void ClearCurrentTarget() { TargetEnemy = nullptr; }
	AActor* GetCurrentTarget() const { return TargetEnemy; }

	// --- Getters for BT nodes ---

	FName GetSenseTag() const { return SenseTag; }
	float GetAttackRange() const { return AttackRange; }
	float GetLoseTargetRange() const { return LoseTargetRange; }
	float GetPatrolRadius() const { return PatrolRadius; }
	bool HasLineOfSightToTarget() const;

	// --- Behavior Tree ---

	UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	bool bUseBehaviorTree = true;

	UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

#if 0 // State Machine
	void SetState(EAIState NewState);
	void TickPatrol(float DeltaTime);
	void TickChase(float DeltaTime);
	void TickAttack(float DeltaTime);
	float GetDistanceToTarget() const;
	FVector GetRandomPatrolPoint() const;
#endif

	// --- Perception ---

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnPerceptionForgotten(AActor* Actor);

	// --- Death ---

	UFUNCTION()
	void OnPawnDeath();
};
