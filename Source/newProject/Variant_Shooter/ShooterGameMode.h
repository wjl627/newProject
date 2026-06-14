// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameMode.generated.h"

class AShooterNPC;

/**
 *  Simple GameMode for a first person shooter game
 *  Manages team and player scores, enemy spawning
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

protected:

	/** Map of scores by team ID */
	TMap<uint8, int32> TeamScores;

	/** Map of individual player scores by controller */
	TMap<AController*, int32> PlayerScores;

	/** Points awarded per enemy kill */
	UPROPERTY(EditAnywhere, Category="Scoring")
	int32 KillScoreAmount = 100;

	/** Desired number of enemies alive at once */
	UPROPERTY(EditAnywhere, Category="Spawning")
	int32 MaxEnemyCount = 5;

	/** Enemy class to spawn */
	UPROPERTY(EditAnywhere, Category="Spawning")
	TSubclassOf<AShooterNPC> EnemyClass;

	/** Interval between spawn checks */
	UPROPERTY(EditAnywhere, Category="Spawning", meta = (ClampMin = 0.5f, ClampMax = 30, Units = "s"))
	float SpawnCheckInterval = 3.0f;

	/** Match duration in seconds */
	UPROPERTY(EditAnywhere, Category="Match", meta = (ClampMin = 60, ClampMax = 3600, Units = "s"))
	float MatchDuration = 900.0f;

	/** Remaining match time in seconds */
	float RemainingTime = 0.0f;

	FTimerHandle SpawnTimer;
	FTimerHandle MatchTimer;

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Periodically checks enemy count and spawns if below threshold */
	void CheckAndSpawnEnemies();

	/** Called every second to tick the match timer */
	void TickMatchTime();

	/** Ends the match and announces the winner */
	void EndMatch();

public:

	/** Increases the score for the given team */
	void IncrementTeamScore(uint8 TeamByte);

	/** Awards kill score to the player who killed an enemy */
	void AddKillScore(AController* Killer);

	/** Halves the dead player's score */
	void OnPlayerDeath(AController* DeadPlayer);

	/** Returns the current score for a player controller */
	int32 GetPlayerScore(AController* Player) const;
};
