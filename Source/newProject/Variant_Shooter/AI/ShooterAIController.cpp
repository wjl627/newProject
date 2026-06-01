// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/AI/ShooterAIController.h"
#include "ShooterNPC.h"
#include "Perception/AIPerceptionComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"

AShooterAIController::AShooterAIController()
{
	PrimaryActorTick.bCanEverTick = true;

	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
	AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AShooterAIController::OnPerceptionUpdated);
	AIPerception->OnTargetPerceptionForgotten.AddDynamic(this, &AShooterAIController::OnPerceptionForgotten);
}

void AShooterAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	PossessedNPC = Cast<AShooterNPC>(InPawn);
	if (PossessedNPC)
	{
		// enable controller-driven yaw so the NPC faces its focus target
		PossessedNPC->bUseControllerRotationYaw = true;

		PossessedNPC->Tags.Add(TeamTag);
		PossessedNPC->OnPawnDeath.AddDynamic(this, &AShooterAIController::OnPawnDeath);

		SetState(EAIState::Patrol);
	}
}

void AShooterAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!PossessedNPC)
	{
		return;
	}

	switch (CurrentState)
	{
	case EAIState::Patrol: TickPatrol(DeltaTime); break;
	case EAIState::Chase:  TickChase(DeltaTime);  break;
	case EAIState::Attack: TickAttack(DeltaTime); break;
	}
}

// -----------------------------------------------------------------------------
// State machine
// -----------------------------------------------------------------------------

void AShooterAIController::SetState(EAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	// Exit current state
	switch (CurrentState)
	{
	case EAIState::Attack:
		if (PossessedNPC)
		{
			PossessedNPC->StopShooting();
		}
		ClearFocus(EAIFocusPriority::Gameplay);
		break;
	case EAIState::Chase:
		StopMovement();
		break;
	case EAIState::Patrol:
		GetWorld()->GetTimerManager().ClearTimer(PatrolWaitTimer);
		bIsWaitingAtPatrol = false;
		break;
	}

	CurrentState = NewState;
}

// -----------------------------------------------------------------------------
// Patrol
// -----------------------------------------------------------------------------

void AShooterAIController::TickPatrol(float DeltaTime)
{
	// If we sense a target during patrol, immediately switch to chase
	if (TargetEnemy)
	{
		SetState(EAIState::Chase);
		return;
	}

	// Still moving to the last patrol point — wait for arrival
	if (GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	// Already waiting at a patrol point
	if (bIsWaitingAtPatrol)
	{
		return;
	}

	// Arrived — wait before picking the next point
	bIsWaitingAtPatrol = true;
	const float WaitTime = FMath::RandRange(MinPatrolWait, MaxPatrolWait);

	GetWorld()->GetTimerManager().SetTimer(
		PatrolWaitTimer,
		[this]()
		{
			bIsWaitingAtPatrol = false;

			if (CurrentState == EAIState::Patrol && !TargetEnemy)
			{
				const FVector PatrolPoint = GetRandomPatrolPoint();
				MoveToLocation(PatrolPoint);
			}
		},
		WaitTime,
		false
	);
}

// -----------------------------------------------------------------------------
// Chase
// -----------------------------------------------------------------------------

void AShooterAIController::TickChase(float DeltaTime)
{
	if (!TargetEnemy)
	{
		SetState(EAIState::Patrol);
		return;
	}

	const float Dist = GetDistanceToTarget();

	// Lost contact
	if (Dist > LoseTargetRange)
	{
		TargetEnemy = nullptr;
		SetState(EAIState::Patrol);
		return;
	}

	// Close enough and can see — attack
	if (Dist <= AttackRange && HasLineOfSightToTarget())
	{
		SetState(EAIState::Attack);
		return;
	}

	// Keep closing in
	MoveToActor(TargetEnemy);
}

// -----------------------------------------------------------------------------
// Attack
// -----------------------------------------------------------------------------

void AShooterAIController::TickAttack(float DeltaTime)
{
	if (!TargetEnemy)
	{
		SetState(EAIState::Patrol);
		return;
	}

	const float Dist = GetDistanceToTarget();

	// Target escaped
	if (Dist > LoseTargetRange)
	{
		TargetEnemy = nullptr;
		SetState(EAIState::Patrol);
		return;
	}

	// Lost advantage — go back to chasing
	if (Dist > AttackRange || !HasLineOfSightToTarget())
	{
		SetState(EAIState::Chase);
		return;
	}

	// Engage: stop, face, shoot
	StopMovement();
	SetFocus(TargetEnemy);

	if (PossessedNPC)
	{
		PossessedNPC->StartShooting(TargetEnemy);
	}
}

// -----------------------------------------------------------------------------
// Helpers
// -----------------------------------------------------------------------------

bool AShooterAIController::HasLineOfSightToTarget() const
{
	if (!TargetEnemy || !PossessedNPC)
	{
		return false;
	}

	const FVector Start = PossessedNPC->GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = TargetEnemy->GetActorLocation();

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PossessedNPC);
	QueryParams.AddIgnoredActor(TargetEnemy);

	FHitResult OutHit;
	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	return !OutHit.bBlockingHit;
}

float AShooterAIController::GetDistanceToTarget() const
{
	if (!TargetEnemy || !PossessedNPC)
	{
		return MAX_FLT;
	}
	return FVector::Dist(PossessedNPC->GetActorLocation(), TargetEnemy->GetActorLocation());
}

FVector AShooterAIController::GetRandomPatrolPoint() const
{
	if (!PossessedNPC)
	{
		return FVector::ZeroVector;
	}

	const FVector Origin = PossessedNPC->GetActorLocation();

	if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld()))
	{
		FNavLocation NavLocation;
		if (NavSys->GetRandomReachablePointInRadius(Origin, PatrolRadius, NavLocation))
		{
			return NavLocation.Location;
		}
	}

	// Fallback when navmesh is not available
	FVector RandomOffset = FMath::VRand() * FMath::RandRange(100.0f, PatrolRadius);
	RandomOffset.Z = 0.0f;
	return Origin + RandomOffset;
}

// -----------------------------------------------------------------------------
// Perception
// -----------------------------------------------------------------------------

void AShooterAIController::OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	if (Stimulus.WasSuccessfullySensed() && Actor->ActorHasTag(SenseTag))
	{
		TargetEnemy = Actor;
	}

	OnShooterPerceptionUpdated.ExecuteIfBound(Actor, Stimulus);
}

void AShooterAIController::OnPerceptionForgotten(AActor* Actor)
{
	if (Actor == TargetEnemy)
	{
		TargetEnemy = nullptr;
	}

	OnShooterPerceptionForgotten.ExecuteIfBound(Actor);
}

// -----------------------------------------------------------------------------
// Death
// -----------------------------------------------------------------------------

void AShooterAIController::OnPawnDeath()
{
	StopMovement();
	GetWorld()->GetTimerManager().ClearTimer(PatrolWaitTimer);

	if (PossessedNPC)
	{
		PossessedNPC->StopShooting();
		PossessedNPC->OnPawnDeath.RemoveDynamic(this, &AShooterAIController::OnPawnDeath);
		PossessedNPC = nullptr;
	}

	UnPossess();
	Destroy();
}
