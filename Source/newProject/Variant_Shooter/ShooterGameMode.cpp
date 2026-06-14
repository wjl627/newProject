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

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Start periodic enemy spawn checks
	GetWorld()->GetTimerManager().SetTimer(
		SpawnTimer,
		this,
		&AShooterGameMode::CheckAndSpawnEnemies,
		SpawnCheckInterval,
		true
	);

	// Start match countdown
	RemainingTime = MatchDuration;
	GetWorld()->GetTimerManager().SetTimer(
		MatchTimer,
		this,
		&AShooterGameMode::TickMatchTime,
		1.0f,
		true
	);
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

	// broadcast team score to all player controllers
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

	// Grant kill rewards (ammo + random buffs) to the killer
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

	// Count alive NPCs
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

	// Find spawn points
	TArray<AActor*> SpawnPoints;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPCSpawner::StaticClass(), SpawnPoints);

	if (SpawnPoints.Num() == 0)
	{
		return;
	}

	// Spawn one enemy at a random spawn point
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

	// Send remaining time to all players
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

	// Collect and sort player scores
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

	// Build ranking text for display (top 3)
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

	if (Entries.Num() == 0)
	{
		RankingText += TEXT("No players scored");
	}

	UE_LOG(LognewProject, Warning, TEXT("Ranking:\n%s"), *RankingText);

	// Send same ranking to ALL players + freeze input
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get()))
		{
			PC->Client_ShowMatchRanking(RankingText);
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
			if (APawn* Pawn = PC->GetPawn()) { Pawn->DisableInput(nullptr); }
		}
	}

	// Cleanup
	TArray<AActor*> AllNPCs;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AShooterNPC::StaticClass(), AllNPCs);
	for (AActor* Actor : AllNPCs) { Actor->Destroy(); }
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);

	FTimerHandle RestartTimer;
	GetWorld()->GetTimerManager().SetTimer(RestartTimer, [this]() { GetWorld()->ServerTravel(TEXT("?Restart"), true); }, 15.0f, false);
}
