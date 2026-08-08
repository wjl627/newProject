// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "ShooterPlayerController.h"
#include "ShooterCharacter.h"
#include "AI/ShooterNPC.h"
#include "AI/ShooterNPCSpawner.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"
#include "newProject.h"

AShooterGameMode::AShooterGameMode()
{
	GameStateClass = AShooterGameState::StaticClass();
}

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// 进入大厅状态——GameState 会复制到所有客户端
	if (AShooterGameState* GS = GetGameState<AShooterGameState>())
	{
		GS->SetMatchState(EMatchState::Lobby);
	}
}

void AShooterGameMode::StartMatch()
{
	AShooterGameState* GS = GetGameState<AShooterGameState>();
	if (!HasAuthority() || !GS || GS->GetMatchState() != EMatchState::Lobby)
	{
		return;
	}

	GS->SetMatchState(EMatchState::InProgress);

	// 启动周期性敌人生成检查
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimer,
		this,
		&AShooterGameMode::CheckAndSpawnEnemies,
		SpawnCheckInterval,
		true
	);

	// 开始比赛倒计时
	RemainingTime = MatchDuration;
	GetWorld()->GetTimerManager().SetTimer(
		MatchTimer,
		this,
		&AShooterGameMode::TickMatchTime,
		1.0f,
		true
	);

	UE_LOG(LognewProject, Log, TEXT("Match started! Duration: %.0fs"), MatchDuration);
}

void AShooterGameMode::IncrementTeamScore(uint8 TeamByte)
{
	int32 Score = 0;
	if (int32* FoundScore = TeamScores.Find(TeamByte))
	{
		Score = *FoundScore;
	}

	++Score;
	TeamScores.Add(TeamByte, Score);

	// 向所有玩家控制器广播队伍分数
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get()))
		{
			PC->Client_UpdateTeamScore(TeamByte, Score);
		}
	}
}

void AShooterGameMode::AddKillScore(AController* Killer)
{
	if (!Cast<APlayerController>(Killer))
	{
		return;
	}

	// 向击杀者发放击杀奖励（弹药 + 随机增益）
	if (AShooterCharacter* KillerChar = Cast<AShooterCharacter>(Killer->GetPawn()))
	{
		KillerChar->GrantKillRewards();
	}

	int32& Score = PlayerScores.FindOrAdd(Killer, 0);
	const float ScoreMult = Cast<AShooterCharacter>(Killer->GetPawn())
		? Cast<AShooterCharacter>(Killer->GetPawn())->GetScoreMultiplier()
		: 1.0f;
	Score += FMath::RoundToInt(KillScoreAmount * ScoreMult);

	if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(Killer))
	{
		PC->Client_UpdatePlayerScore(Score);
	}
}

void AShooterGameMode::OnPlayerDeath(AController* DeadPlayer)
{
	if (!Cast<APlayerController>(DeadPlayer))
	{
		return;
	}

	int32& Score = PlayerScores.FindOrAdd(DeadPlayer, 0);
	Score = FMath::Max(0, Score / 2);

	if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(DeadPlayer))
	{
		PC->Client_UpdatePlayerScore(Score);
	}
}

void AShooterGameMode::CheckAndSpawnEnemies()
{
	if (!HasAuthority() || !EnemyClass)
	{
		return;
	}

	// 统计存活 NPC 数量
	int32 AliveCount = 0;
	TArray<AActor*> AllNPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPC::StaticClass(), AllNPCs);
	for (AActor* Actor : AllNPCs)
	{
		if (AShooterNPC* NPC = Cast<AShooterNPC>(Actor))
		{
			if (!NPC->IsDead())
			{
				++AliveCount;
			}
		}
	}

	if (AliveCount >= MaxEnemyCount)
	{
		return;
	}

	// 查找生成点
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPCSpawner::StaticClass(), SpawnPoints);

	if (SpawnPoints.Num() == 0)
	{
		return;
	}

	// 在随机生成点生成一个敌人
	const AShooterNPCSpawner* Spawner = Cast<AShooterNPCSpawner>(SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)]);
	const FTransform SpawnTransform = Spawner->GetSpawnTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	GetWorld()->SpawnActor<AShooterNPC>(EnemyClass, SpawnTransform, SpawnParams);
}

int32 AShooterGameMode::GetPlayerScore(AController* Player) const
{
	if (const int32* Score = PlayerScores.Find(Player))
	{
		return *Score;
	}
	return 0;
}

void AShooterGameMode::TickMatchTime()
{
	if (!HasAuthority())
	{
		return;
	}

	RemainingTime = FMath::Max(0.0f, RemainingTime - 1.0f);

	// 在 GameState 上更新剩余时间（复制到所有客户端）
	if (AShooterGameState* GS = GetGameState<AShooterGameState>())
	{
		GS->SetRemainingTime(RemainingTime);
	}

	// 向所有玩家发送剩余时间（用于 HUD 显示的客户端 RPC）
	const int32 RemainingSeconds = FMath::CeilToInt(RemainingTime);
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get()))
		{
			PC->Client_UpdateMatchTime(RemainingSeconds);
		}
	}

	if (RemainingTime <= 0.0f)
	{
		GetWorld()->GetTimerManager().ClearTimer(MatchTimer);
		EndMatch();
		UE_LOG(LognewProject, Warning, TEXT("Match time expired! Calling EndMatch..."));
	}
}


void AShooterGameMode::EndMatch()
{
	UE_LOG(LognewProject, Warning, TEXT("=== EndMatch! PlayerScores has %d entries ==="), PlayerScores.Num());

	// 收集并排序玩家分数
	struct FPlayerEntry
	{
		AController* Controller = nullptr;
		int32 Score = 0;
	};
	TArray<FPlayerEntry> Entries;

	for (const TPair<AController*, int32>& Pair : PlayerScores)
	{
		if (Cast<APlayerController>(Pair.Key))
		{
			Entries.Add({Pair.Key, Pair.Value});
		}
	}

	Entries.Sort([](const FPlayerEntry& A, const FPlayerEntry& B) { return A.Score > B.Score; });

	// 生成用于显示的排名文本（前三名）
	FString RankingText = TEXT("=== MATCH RESULTS ===\n\n");
	const TCHAR* PlaceNames[] = {TEXT("1st"), TEXT("2nd"), TEXT("3rd")};

	for (int32 i = 0; i < FMath::Min(Entries.Num(), 3); ++i)
	{
		FString PlayerName = TEXT("Player");
		if (Entries[i].Controller && Entries[i].Controller->GetPawn())
		{
			PlayerName = Entries[i].Controller->GetPawn()->GetName();
		}
		RankingText += FString::Printf(TEXT("%s: %s  (%d pts)\n"), PlaceNames[i], *PlayerName, Entries[i].Score);
	}

	UE_LOG(LognewProject, Warning, TEXT("Ranking:\n%s"), *RankingText);

	// 在 SetMatchState 之前发送排名，以便小部件在游戏模式下更新
	int32 PCCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get()))
		{
			++PCCount;
			PC->Client_ShowMatchRanking(RankingText);
		}
	}
	UE_LOG(LognewProject, Warning, TEXT("EndMatch: Sent ranking to %d player controllers"), PCCount);

	// 在排名 RPC 之后切换到 Ended 状态（触发客户端的输入屏蔽）
	AShooterGameState* GS = GetGameState<AShooterGameState>();
	if (GS)
	{
		GS->SetMatchState(EMatchState::Ended);
	}

	// 清理
	TArray<AActor*> AllNPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPC::StaticClass(), AllNPCs);
	for (AActor* Actor : AllNPCs) { Actor->Destroy(); }
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	FTimerHandle RestartTimer;
	GetWorld()->GetTimerManager().SetTimer(RestartTimer, [this]() { GetWorld()->ServerTravel(TEXT("?Restart"), true); }, 15.0f, false);
}
