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
#include "ShooterGameMode.h"
#include "ShooterGameInstance.h"
#include "Engine/Engine.h"
#include "newProject.h"
#include "Widgets/Input/SVirtualJoystick.h"

void AShooterPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ENetMode NetMode = GetWorld()->GetNetMode();
	bIsHost = (NetMode != NM_Client);

	BlockAllGameInput();

	if (!IsLocalPlayerController())
	{
		return;
	}

	// Create game HUD widgets (hidden during menus, shown during gameplay)
	if (ShouldUseTouchControls())
	{
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);
		if (MobileControlsWidget)
		{
			MobileControlsWidget->AddToPlayerScreen(0);
		}
	}

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

	ScoreboardUI = CreateWidget<UShooterUI>(this, ScoreboardUIClass);
	if (ScoreboardUI)
	{
		ScoreboardUI->AddToViewport(1);
	}

	TSubclassOf<UBuffMessageUI> BuffClass = BuffMessageUIClass;
	if (!BuffClass)
	{
		BuffClass = UBuffMessageUI::StaticClass();
	}
	BuffMessageWidget = CreateWidget<UBuffMessageUI>(this, BuffClass);
	if (BuffMessageWidget)
	{
		BuffMessageWidget->AddToViewport(10);
	}

	// Standalone = first launch (main menu). ListenServer/Client = show lobby (match starts when host presses Enter).
	if (NetMode == NM_Standalone)
	{
		ShowMainMenu();
	}
	else if (NetMode == NM_ListenServer || NetMode == NM_Client)
	{
		ShowLobby();
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
		// Bind Enter key for lobby start
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AShooterPlayerController::OnLobbyStartPressed);
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
	if (IsLocalPlayerController() && IsValid(BulletCounterUI))
	{
		BulletCounterUI->SetAmmo(0, 0);
	}

	if (!HasAuthority())
	{
		return;
	}

	TArray<AActor*> PlayerStarts;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), PlayerStarts);
	if (PlayerStarts.Num() == 0)
	{
		return;
	}

	// Gather alive player positions to avoid spawning on top of them
	TArray<FVector> AlivePositions;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* OtherPC = It->Get())
		{
			if (OtherPC != this && OtherPC->GetPawn() && !OtherPC->GetPawn()->ActorHasTag(TEXT("Dead")))
			{
				AlivePositions.Add(OtherPC->GetPawn()->GetActorLocation());
			}
		}
	}

	// Pick the PlayerStart farthest from any alive player
	AActor* BestStart = PlayerStarts[0];
	float BestMinDist = -1.0f;
	for (AActor* Start : PlayerStarts)
	{
		float MinDistSq = MAX_FLT;
		for (const FVector& Loc : AlivePositions)
		{
			MinDistSq = FMath::Min(MinDistSq, FVector::DistSquared(Start->GetActorLocation(), Loc));
		}
		if (MinDistSq > BestMinDist)
		{
			BestMinDist = MinDistSq;
			BestStart = Start;
		}
	}

	const FTransform SpawnTransform = BestStart->GetActorTransform();
	if (AShooterCharacter* RespawnedCharacter = GetWorld()->SpawnActor<AShooterCharacter>(CharacterClass, SpawnTransform))
	{
		Possess(RespawnedCharacter);
	}
}

void AShooterPlayerController::OnBulletCountUpdated(int32 MagazineSize, int32 Bullets)
{
	if (bMatchStarted && BulletCounterUI)
	{
		BulletCounterUI->SetAmmo(MagazineSize, Bullets);
	}
}

void AShooterPlayerController::OnPawnDamaged(float LifePercent)
{
	if (bMatchStarted && BulletCounterUI)
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
	if (ScoreboardUI)
	{
		ScoreboardUI->BP_UpdateMatchTime(RemainingSeconds);
	}
}

void AShooterPlayerController::Client_ShowMatchRanking_Implementation(const FString& RankingText)
{
	if (ScoreboardUI)
	{
		ScoreboardUI->BP_ShowMatchRanking(RankingText);
	}

	BlockAllGameInput();
	SetInputMode(FInputModeUIOnly());
	bShowMouseCursor = true;
}

void AShooterPlayerController::OnMatchStateChanged(EMatchState NewState)
{
	switch (NewState)
	{
	case EMatchState::Lobby:
		BlockAllGameInput();
		break;

	case EMatchState::InProgress:
		bMatchStarted = true;
		// Remove menu / lobby widgets
		if (MainMenuWidget)
		{
			MainMenuWidget->RemoveFromParent();
			MainMenuWidget = nullptr;
		}
		if (LobbyWidget)
		{
			LobbyWidget->RemoveFromParent();
			LobbyWidget = nullptr;
		}
		SetInputMode(FInputModeGameOnly());
		bShowMouseCursor = false;
		EnableGameInput();
		break;

	case EMatchState::Ended:
		BlockAllGameInput();
		SetInputMode(FInputModeUIOnly());
		bShowMouseCursor = true;
		break;
	}
}

void AShooterPlayerController::Server_StartMatch_Implementation()
{
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->StartMatch();
	}
}

void AShooterPlayerController::BlockAllGameInput()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : DefaultMappingContexts)
		{
			Subsystem->RemoveMappingContext(Context);
		}
		for (UInputMappingContext* Context : MobileExcludedMappingContexts)
		{
			Subsystem->RemoveMappingContext(Context);
		}
	}
}

void AShooterPlayerController::EnableGameInput()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		for (UInputMappingContext* Context : DefaultMappingContexts)
		{
			Subsystem->AddMappingContext(Context, 0);
		}
		if (!ShouldUseTouchControls())
		{
			for (UInputMappingContext* Context : MobileExcludedMappingContexts)
			{
				Subsystem->AddMappingContext(Context, 0);
			}
		}
	}
}

void AShooterPlayerController::ShowMainMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (LobbyWidget)
	{
		LobbyWidget->RemoveFromParent();
		LobbyWidget = nullptr;
	}

	if (MainMenuWidgetClass && !MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UUserWidget>(this, MainMenuWidgetClass);
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport(100);
		}
	}

	SetInputMode(FInputModeUIOnly());
	bShowMouseCursor = true;
}

void AShooterPlayerController::ShowLobby()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	if (MainMenuWidget)
	{
		MainMenuWidget->RemoveFromParent();
		MainMenuWidget = nullptr;
	}

	if (LobbyWidgetClass && !LobbyWidget)
	{
		LobbyWidget = CreateWidget<UUserWidget>(this, LobbyWidgetClass);
		if (LobbyWidget)
		{
			LobbyWidget->AddToViewport(100);
		}
	}

	SetInputMode(FInputModeUIOnly());
	bShowMouseCursor = true;
}

void AShooterPlayerController::OnLobbyStartPressed()
{
	if (HasAuthority())
	{
		Server_StartMatch();
	}
}
