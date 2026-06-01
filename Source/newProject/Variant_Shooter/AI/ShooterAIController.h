// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShooterAIController.generated.h"

class UAIPerceptionComponent;
class AShooterNPC;
struct FAIStimulus;

DECLARE_DELEGATE_TwoParams(FShooterPerceptionUpdatedDelegate, AActor*, const FAIStimulus&);
DECLARE_DELEGATE_OneParam(FShooterPerceptionForgottenDelegate, AActor*);

/** AI behavior states */
UENUM()
enum class EAIState : uint8
{
	/** Wandering between random points, looking for enemies */
	Patrol,
	/** Moving towards a sensed target */
	Chase,
	/** In range and shooting at target */
	Attack
};

/**
 *  AI Controller for shooter enemy NPCs.
 *  Uses a C++ state machine: Patrol → Chase → Attack.
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterAIController : public AAIController
{
	GENERATED_BODY()

	/** Detects other actors through sight and hearing */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionComponent* AIPerception;

protected:

	/** Tag added to the possessed pawn */
	UPROPERTY(EditAnywhere, Category="Shooter")
	FName TeamTag = FName("Enemy");

	/** Only actors with this tag will be treated as enemies */
	UPROPERTY(EditAnywhere, Category="Shooter|Perception")
	FName SenseTag = FName("Player");

	// --- Patrol ---

	/** Radius around the spawn point for random patrol destinations */
	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, Units = "cm"))
	float PatrolRadius = 1500.0f;

	/** Minimum wait time at a patrol point */
	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float MinPatrolWait = 1.0f;

	/** Maximum wait time at a patrol point */
	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float MaxPatrolWait = 3.0f;

	// --- Combat ---

	/** Distance at which to stop chasing and start attacking */
	UPROPERTY(EditAnywhere, Category="Shooter|Combat", meta = (ClampMin = 0, Units = "cm"))
	float AttackRange = 2000.0f;

	/** Distance beyond which the target is considered lost */
	UPROPERTY(EditAnywhere, Category="Shooter|Combat", meta = (ClampMin = 0, Units = "cm"))
	float LoseTargetRange = 4000.0f;

	// --- Runtime state ---

	/** Current behavior state */
	EAIState CurrentState = EAIState::Patrol;

	/** Currently sensed enemy target */
	TObjectPtr<AActor> TargetEnemy;

	/** Cached reference to the possessed NPC pawn */
	TObjectPtr<AShooterNPC> PossessedNPC;

	/** Timer handle for waiting at patrol points */
	FTimerHandle PatrolWaitTimer;

	/** True while waiting at a patrol point */
	bool bIsWaitingAtPatrol = false;

public:

	AShooterAIController();

	/** Perception delegate hooks — kept for compatibility with ShooterStateTreeUtility */
	FShooterPerceptionUpdatedDelegate OnShooterPerceptionUpdated;
	FShooterPerceptionForgottenDelegate OnShooterPerceptionForgotten;

	/** Sets the targeted enemy */
	void SetCurrentTarget(AActor* Target) { TargetEnemy = Target; }

	/** Clears the targeted enemy */
	void ClearCurrentTarget() { TargetEnemy = nullptr; }

	/** Returns the targeted enemy */
	AActor* GetCurrentTarget() const { return TargetEnemy; }

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

	// --- State machine ---

	/** Transition to a new behavior state */
	void SetState(EAIState NewState);

	void TickPatrol(float DeltaTime);
	void TickChase(float DeltaTime);
	void TickAttack(float DeltaTime);

	// --- Helpers ---

	bool HasLineOfSightToTarget() const;
	float GetDistanceToTarget() const;
	FVector GetRandomPatrolPoint() const;

	// --- Perception callbacks ---

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnPerceptionForgotten(AActor* Actor);

	// --- Death ---

	UFUNCTION()
	void OnPawnDeath();
};
