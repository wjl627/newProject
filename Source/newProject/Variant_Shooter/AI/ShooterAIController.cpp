// Copyright Epic Games, Inc. All Rights Reserved.

#include "Variant_Shooter/AI/ShooterAIController.h"
#include "ShooterNPC.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Character.h"
#include "BehaviorTree/BehaviorTree.h"
#include "Engine/World.h"

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

		// 应用运行时视觉配置，让敌人能够看得很远以发动攻击，
		// 并且不会跟丢靠近/侧面的目标。
		if (UAIPerceptionComponent* PerceptionComp = GetPerceptionComponent())
		{
			if (UAISenseConfig_Sight* SightConfig = PerceptionComp->GetSenseConfig<UAISenseConfig_Sight>())
			{
				SightConfig->SightRadius = FMath::Max(PerceptionSightRadius, AttackRange);
				SightConfig->LoseSightRadius = FMath::Max(PerceptionLoseSightRadius, PerceptionSightRadius);
				SightConfig->PeripheralVisionAngleDegrees = PerceptionSightAngle;
				SightConfig->SetMaxAge(PerceptionMaxAge);
				PerceptionComp->RequestStimuliListenerUpdate();
			}
		}

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
// 辅助函数
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

void AShooterAIController::UpdateLastKnownTargetLocation(const FVector& Location)
{
	LastKnownTargetLocation = Location;
	if (UWorld* World = GetWorld())
	{
		LastTargetSeenTime = World->GetTimeSeconds();
	}
}

bool AShooterAIController::GetLastKnownTargetLocation(FVector& OutLocation, float MaxAge) const
{
	if (GetTimeSinceLastSeenTarget() <= MaxAge)
	{
		OutLocation = LastKnownTargetLocation;
		return true;
	}
	return false;
}

float AShooterAIController::GetTimeSinceLastSeenTarget() const
{
	if (const UWorld* World = GetWorld())
	{
		return World->GetTimeSeconds() - LastTargetSeenTime;
	}
	return MAX_FLT;
}

// -----------------------------------------------------------------------------
// 感知
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
// 死亡
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
// 状态机——已弃用，仅作参考保留
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

#endif // 状态机
