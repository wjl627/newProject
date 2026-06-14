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
#include "BuffMessageUI.h"
#include "MatchResultOverlay.h"
#include "ShooterGameMode.h"
#include "Engine/Engine.h"
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

		BuffMessageWidget = CreateWidget<UBuffMessageUI>(this, UBuffMessageUI::StaticClass());
		if (BuffMessageWidget)
		{
			BuffMessageWidget->AddToPlayerScreen(10);
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

		// Re-send the current score to the client after respawn
		if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
		{
			const int32 Score = GM->GetPlayerScore(this);
			Client_UpdatePlayerScore(Score);
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

void AShooterPlayerController::Client_ShowBuffMessage_Implementation(const FString& Message, FColor Color)
{
	if (BuffMessageWidget)
	{
		BuffMessageWidget->Show(Message, Color);
	}
}

void AShooterPlayerController::Client_UpdateMatchTime_Implementation(int32 RemainingSeconds)
{
	const int32 Minutes = RemainingSeconds / 60;
	const int32 Seconds = RemainingSeconds % 60;
	const FString TimeStr = FString::Printf(TEXT("Time: %02d:%02d"), Minutes, Seconds);

	GEngine->AddOnScreenDebugMessage(1, 1.5f, FColor::White, TimeStr);

	if (ScoreboardUI)
	{
		ScoreboardUI->BP_UpdateMatchTime(RemainingSeconds);
	}
}

void AShooterPlayerController::Client_AnnounceMatchResult_Implementation(bool bIsWinner, const FString& WinnerName, int32 WinnerScore, int32 MyScore)
{
	const FString ResultLine = bIsWinner
		? FString::Printf(TEXT("YOU WIN!  Score: %d"), MyScore)
		: FString::Printf(TEXT("GAME OVER  Winner: %s (%d pts)  Your Score: %d"), *WinnerName, WinnerScore, MyScore);

	UE_LOG(LognewProject, Warning, TEXT("Match Result: %s"), *ResultLine);

	// Hide the bullet counter UI
	if (BulletCounterUI)
	{
		BulletCounterUI->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Create a fresh result overlay widget (no blueprint dependency)
	if (UMatchResultOverlay* ResultOverlay = CreateWidget<UMatchResultOverlay>(this, UMatchResultOverlay::StaticClass()))
	{
		ResultOverlay->SetResult(ResultLine, bIsWinner ? FColor::Yellow : FColor::Red);
		ResultOverlay->AddToViewport(100);
	}

	if (ScoreboardUI)
	{
		ScoreboardUI->BP_AnnounceMatchResult(bIsWinner, WinnerName, WinnerScore, MyScore);
	}

	// Block all game input on this client
	SetInputMode(FInputModeUIOnly());
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->DisableInput(this);
	}
}

void AShooterPlayerController::Client_ShowMatchRanking_Implementation(const FString& RankingText)
{
	if (BulletCounterUI)
	{
		BulletCounterUI->ShowMatchRanking(RankingText);
	}

	// Debug display (bypasses UMG, visible even if UMG fails)
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(999, 15.0f, FColor::Yellow, RankingText);
	}

	// Block input on this client
	SetInputMode(FInputModeUIOnly());
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);
	bShowMouseCursor = true;
	if (APawn* MyPawn = GetPawn())
	{
		MyPawn->DisableInput(this);
	}
}
