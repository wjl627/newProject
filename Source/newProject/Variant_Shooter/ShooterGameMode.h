// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

/**
 *  Simple GameMode for a first person shooter game
 *  Manages team and player scores
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	/** Map of scores by team ID */
	TMap<uint8, int32> TeamScores;

	/** Individual player score */
	int32 PlayerScore = 0;

	/** Points awarded per enemy kill */
	UPROPERTY(EditAnywhere, Category="Scoring")
	int32 KillScoreAmount = 100;

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

public:

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	/** Awards kill score to the player who killed an enemy */
	void AddKillScore(AController* Killer);

	/** Halves the player's score on death */
	void OnPlayerDeath();
};
