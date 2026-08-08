// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterNPCSpawner.generated.h"

class UCapsuleComponent;
class UArrowComponent;
class AShooterNPC;

UCLASS()
class NEWPROJECT_API AShooterNPCSpawner : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UCapsuleComponent* SpawnCapsule;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
	UArrowComponent* SpawnDirection;

protected:

	/** 要生成的 NPC 类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC Spawner")
	TSubclassOf<AShooterNPC> NPCClass;

	/** 游戏开始时生成第一个 NPC 前的等待时间 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float InitialSpawnDelay = 5.0f;

	/** 要生成的 NPC 数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 100))
	int32 SpawnCount = 1;

	/** 当前 NPC 死亡后，生成下一个 NPC 前的等待时间 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC Spawner", meta = (ClampMin = 0, ClampMax = 10))
	float RespawnDelay = 5.0f;

	/** 如果为 false，生成器仅作为 GameMode 的生成点 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="NPC Spawner")
	bool bAutoSpawn = false;

	/** 延迟生成 NPC 的计时器 */
	FTimerHandle SpawnTimer;

public:

	/** 构造函数 */
	AShooterNPCSpawner();

	/** 返回世界空间中的生成变换 */
	FTransform GetSpawnTransform() const;

public:

	/** 初始化 */
	virtual void BeginPlay() override;

	/** 清理 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** 生成一个 NPC 并订阅其死亡事件 */
	void SpawnNPC();

	/** 当生成的 NPC 死亡时调用 */
	UFUNCTION()
	void OnNPCDied();

};
