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

	// 创建游戏 HUD 小部件（菜单期间隐藏，游戏期间显示）
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

	// 单机 = 首次启动（主菜单）。监听服务器/客户端 = 显示大厅（房主按 Enter 开始比赛）。
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

	// 仅为本地玩家控制器添加输入映射上下文
	if (IsLocalPlayerController())
	{
		// 添加输入映射上下文
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// 仅在未使用移动触摸输入时添加这些输入映射上下文
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
		// 绑定 Enter 键开始比赛（大厅中）
		InputComponent->BindKey(EKeys::Enter, IE_Pressed, this, &AShooterPlayerController::OnLobbyStartPressed);
	}
}

void AShooterPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// 订阅 Pawn 的 OnDestroyed 委托
	InPawn->OnDestroyed.AddDynamic(this, &AShooterPlayerController::OnPawnDestroyed);

	// 这是射击角色吗？
	if (AShooterCharacter* ShooterCharacter = Cast<AShooterCharacter>(InPawn))
	{
		// 添加玩家标签（服务器权威）
		ShooterCharacter->Tags.Add(PlayerPawnTag);

		// OnPossess 在服务器上运行；监听服务器/单机主机需要在这里设置 HUD
		if (IsLocalPlayerController())
		{
			SetupShooterCharacterUI(ShooterCharacter);
		}

		// 重生后向客户端重新发送当前分数
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

	// 专用客户端在这里接收其 Pawn，而不是在本地玩家控制器的 OnPossess 中
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

	// 如果 OnPossess 和 AcknowledgePossession 都运行，避免重复绑定
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

	// 收集存活玩家的位置，避免生成在他们身上
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

	// 选择离所有存活玩家最远的 PlayerStart
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
	// 我们是否在移动平台上？是否需要强制触控？
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
		// 移除菜单 / 大厅小部件
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
}

void AShooterPlayerController::OnLobbyStartPressed()
{
	if (HasAuthority())
	{
		Server_StartMatch();
	}
}
