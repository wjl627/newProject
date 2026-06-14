// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Shooter/AI/ShooterAIController.h"
#include "ShooterNPC.h"
#include "Perception/AIPerceptionComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BehaviorTree.h"

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
		PossessedNPC->bUseControllerRotationYaw = true;
		PossessedNPC->Tags.Add(TeamTag);
		PossessedNPC->OnPawnDeath.AddDynamic(this, &AShooterAIController::OnPawnDeath);

		if (bUseBehaviorTree && BehaviorTreeAsset)
		{
			RunBehaviorTree(BehaviorTreeAsset);
		}
	}
}

void AShooterAIController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
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
	FVector End = TargetEnemy->GetActorLocation();
	if (const ACharacter* TargetChar = Cast<ACharacter>(TargetEnemy))
	{
		End.Z += TargetChar->BaseEyeHeight * 0.8f;
	}
	else
	{
		End.Z += 88.0f;
	}

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(PossessedNPC);
	QueryParams.AddIgnoredActor(TargetEnemy);

	FHitResult OutHit;
	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	return !OutHit.bBlockingHit;
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

	if (PossessedNPC)
	{
		PossessedNPC->StopShooting();
		PossessedNPC->OnPawnDeath.RemoveDynamic(this, &AShooterAIController::OnPawnDeath);
		PossessedNPC = nullptr;
	}

	UnPossess();
	Destroy();
}

// -----------------------------------------------------------------------------
// State Machine — deprecated, kept for reference
// -----------------------------------------------------------------------------
#if 0

#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

void AShooterAIController::SetState(EAIState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

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

void AShooterAIController::TickPatrol(float DeltaTime)
{
	if (TargetEnemy)
	{
		SetState(EAIState::Chase);
		return;
	}

	if (GetMoveStatus() == EPathFollowingStatus::Moving)
	{
		return;
	}

	if (bIsWaitingAtPatrol)
	{
		return;
	}

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

void AShooterAIController::TickChase(float DeltaTime)
{
	if (!TargetEnemy)
	{
		SetState(EAIState::Patrol);
		return;
	}

	const float Dist = GetDistanceToTarget();

	if (Dist > LoseTargetRange)
	{
		TargetEnemy = nullptr;
		SetState(EAIState::Patrol);
		return;
	}

	if (Dist <= AttackRange && HasLineOfSightToTarget())
	{
		SetState(EAIState::Attack);
		return;
	}

	MoveToActor(TargetEnemy);
}

void AShooterAIController::TickAttack(float DeltaTime)
{
	if (!TargetEnemy)
	{
		SetState(EAIState::Patrol);
		return;
	}

	const float Dist = GetDistanceToTarget();

	if (Dist > LoseTargetRange)
	{
		TargetEnemy = nullptr;
		SetState(EAIState::Patrol);
		return;
	}

	if (Dist > AttackRange || !HasLineOfSightToTarget())
	{
		SetState(EAIState::Chase);
		return;
	}

	StopMovement();
	SetFocus(TargetEnemy);

	if (PossessedNPC)
	{
		PossessedNPC->StartShooting(TargetEnemy);
	}
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

	FVector RandomOffset = FMath::VRand() * FMath::RandRange(100.0f, PatrolRadius);
	RandomOffset.Z = 0.0f;
	return Origin + RandomOffset;
}

#endif // State Machine
