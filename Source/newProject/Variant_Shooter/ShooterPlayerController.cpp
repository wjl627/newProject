// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerStart.h"
#include "ShooterCharacter.h"
#include "ShooterBulletCounterUI.h"
#include "ShooterUI.h"
#include "newProject.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController())
	{
		if (ShouldUseTouchControls())
		{
			// spawn the mobile controls widget
			MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

			if (MobileControlsWidget)
			{
				// add the controls to the player screen
				MobileControlsWidget->AddToPlayerScreen(0);

			} else {

				UE_LOG(LognewProject, Error, TEXT("Could not spawn mobile controls widget."));

			}
		}

		// create the bullet counter widget and add it to the screen
		// fall back to C++ base class if no blueprint class is set
		TSubclassOf<UShooterBulletCounterUI> UIClass = BulletCounterUIClass;
		if (!UIClass)
		{
			UIClass = UShooterBulletCounterUI::StaticClass();
		}

		BulletCounterUI = CreateWidget<UShooterBulletCounterUI>(this, UIClass);

		if (BulletCounterUI)
		{
			BulletCounterUI->AddToPlayerScreen(0);
		}
		else
		{
			UE_LOG(LognewProject, Error, TEXT("Could not spawn bullet counter widget."));
		}

		// create the scoreboard widget and add it to the screen
		ScoreboardUI = CreateWidget<UShooterUI>(this, ScoreboardUIClass);

		if (ScoreboardUI)
		{
			ScoreboardUI->AddToViewport(1);
		}

	}
}

void AShooterPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// add the input mapping contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// subscribe to the pawn's OnDestroyed delegate
	InPawn->OnDestroyed.AddDynamic(this, &AShooterPlayerController::OnPawnDestroyed);

	// is this a shooter character?
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn))
	{
		// add the player tag (server authoritative)
		ShooterCharacter->Tags.Add(PlayerPawnTag);

		// OnPossess runs on the server; listen-server / standalone hosts need HUD setup here
		if (IsLocalPlayerController())
		{
			SetupShooterCharacterUI(ShooterCharacter);
		}
	}
}

void AShooterPlayerController::AcknowledgePossession(APawn* P)
{
	Super::AcknowledgePossession(P);

	// Dedicated clients receive their pawn here, not in OnPossess on the local PC
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(P))
	{
		SetupShooterCharacterUI(ShooterCharacter);
	}
}

void AShooterPlayerController::SetupShooterCharacterUI(AShooterCharacter* ShooterCharacter)
{
	if (!ShooterCharacter || !IsLocalPlayerController())
	{
		return;
	}

	// avoid duplicate bindings if both OnPossess and AcknowledgePossession run
	ShooterCharacter->OnBulletCountUpdated.RemoveDynamic(this, &AShooterPlayerController::OnBulletCountUpdated);
	ShooterCharacter->OnDamaged.RemoveDynamic(this, &AShooterPlayerController::OnPawnDamaged);

	ShooterCharacter->OnBulletCountUpdated.AddDynamic(this, &AShooterPlayerController::OnBulletCountUpdated);
	ShooterCharacter->OnDamaged.AddDynamic(this, &AShooterPlayerController::OnPawnDamaged);

	const float LifePercent = FMath::Max(0.0f, ShooterCharacter->GetMaxHP() > 0.0f
		? ShooterCharacter->GetCurrentHP() / ShooterCharacter->GetMaxHP()
		: 1.0f);
	OnPawnDamaged(LifePercent);
}

void AShooterPlayerController::OnPawnDestroyed(AActor* DestroyedActor)
{
	// reset HUD on the owning client
	if (IsLocalPlayerController() && IsValid(BulletCounterUI))
	{
		BulletCounterUI->SetAmmo(0, 0);
	}

	// respawn is server-authoritative
	if (!HasAuthority())
	{
		return;
	}

	// find the player start
	TArray<AActor*> ActorList;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), ActorList);

	if (ActorList.Num() > 0)
	{
		// select a random player start
		AActor* RandomPlayerStart = ActorList[FMath::RandRange(0, ActorList.Num() - 1)];

		// spawn a character at the player start
		const FTransform SpawnTransform = RandomPlayerStart->GetActorTransform();

		if (AShooterCharacter* RespawnedCharacter = GetWorld()->SpawnActor<AShooterCharacter>(CharacterClass, SpawnTransform))
		{
			// possess the character
			Possess(RespawnedCharacter);
		}
	}
}

void AShooterPlayerController::OnBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	if (BulletCounterUI)
	{
		BulletCounterUI->SetAmmo(MagazineSize, Bullets);
	}
}

void AShooterPlayerController::OnPawnDamaged(float LifePercent)
{
	if (BulletCounterUI)
	{
		BulletCounterUI->SetHealthPercent(LifePercent);
	}
}

bool AShooterPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AShooterPlayerController::Client_UpdatePlayerScore_Implementation(int32 Score)
{
	if (ScoreboardUI)
	{
		ScoreboardUI->BP_UpdatePlayerScore(Score);
	}
}

void AShooterPlayerController::Client_UpdateTeamScore_Implementation(uint8 TeamByte, int32 Score)
{
	if (ScoreboardUI)
	{
		ScoreboardUI->BP_UpdateScore(TeamByte, Score);
	}
}
