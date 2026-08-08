// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ShooterGameState.h"
#include "ShooterGameMode.generated.h"

class AShooterNPC;

/**
 *  第一人称射击游戏的简单 GameMode
 *  管理队伍和玩家分数、敌人生成
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AShooterGameMode();

protected:

	/** 按队伍 ID 记录的分数映射 */
	TMap<uint8, int32> TeamScores;

	/** 按控制器记录的玩家个人分数映射 */
	TMap<AController*, int32> PlayerScores;

	/** 每击杀一个敌人获得的分数 */
	UPROPERTY(EditAnywhere, Category="Scoring")
	int32 KillScoreAmount = 100;

	/** 同时存活的期望敌人数量 */
	UPROPERTY(EditAnywhere, Category="Spawning")
	int32 MaxEnemyCount = 5;

	/** 要生成的敌人类型 */
	UPROPERTY(EditAnywhere, Category="Spawning")
	TSubclassOf<AShooterNPC> EnemyClass;

	/** 生成检查的时间间隔 */
	UPROPERTY(EditAnywhere, Category="Spawning", meta = (ClampMin = 0.5f, ClampMax = 30, Units = "s"))
	float SpawnCheckInterval = 3.0f;

	/** 比赛时长（秒） */
	UPROPERTY(EditAnywhere, Category="Match", meta = (ClampMin = 60, ClampMax = 3600, Units = "s"))
	float MatchDuration = 60.0f;

	/** 剩余比赛时间（秒） */
	float RemainingTime = 0.0f;

	FTimerHandle SpawnTimer;
	FTimerHandle MatchTimer;

protected:

	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 定期检查敌人数量，低于阈值时生成 */
	void CheckAndSpawnEnemies();

	/** 每秒调用一次以更新比赛计时器 */
	void TickMatchTime();

	/** 结束比赛并宣布胜者 */
	void EndMatch();

public:

	/** 开始比赛（由房主从大厅调用） */
	UFUNCTION(BlueprintCallable, Category="Match")
	void StartMatch();

	/** 增加指定队伍的分数 */
	void IncrementTeamScore(uint8 TeamByte);

	/** 向击杀敌人的玩家发放击杀分数 */
	void AddKillScore(AController* Killer);

	/** 将死亡玩家的分数减半 */
	void OnPlayerDeath(AController* DeadPlayer);

	/** 返回指定玩家控制器的当前分数 */
	int32 GetPlayerScore(AController* Player) const;
};
