// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/ShooterGameMode.h"
#include "ShooterPlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

void AShooterGameMode::BeginPlay()
{
	Super::BeginPlay();
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

	PlayerScore += KillScoreAmount;

	if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(Killer))
	{
		PC->Client_UpdatePlayerScore(PlayerScore);
	}
}

void AShooterGameMode::OnPlayerDeath()
{
	PlayerScore = FMath::Max(0, PlayerScore / 2);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(It->Get()))
		{
			PC->Client_UpdatePlayerScore(PlayerScore);
		}
	}
}
