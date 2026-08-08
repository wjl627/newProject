// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/DataTable.h"
#include "Engine/StaticMesh.h"
#include "ShooterPickup.generated.h"

class USphereComponent;
class UPrimitiveComponent;
class AShooterWeapon;

/**
 *  保存一种武器拾取物的信息
 */
USTRUCT(BlueprintType)
struct FWeaponTableRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 拾取物上显示的网格 */
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UStaticMesh> StaticMesh;

	/** 拾取时给予的武器类 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<AShooterWeapon> WeaponToSpawn;
};

/**
 *  射击游戏的简单武器拾取物
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterPickup : public AActor
{
	GENERATED_BODY()

	/** 碰撞球体 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* SphereCollision;

	/** 武器拾取物网格。其网格资源由武器数据表设置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* Mesh;
	
protected:

	/** 拾取武器类型及该拾取物外观的数据 */
	UPROPERTY(EditAnywhere, Category="Pickup")
	FDataTableRowHandle WeaponType;

	/** 拾取时要给予的武器类型。由武器数据表设置。 */
	TSubclassOf<AShooterWeapon> WeaponClass;
	
	/** 重生该拾取物前的等待时间 */
	UPROPERTY(EditAnywhere, Category="Pickup", meta = (ClampMin = 0, ClampMax = 120, Units = "s"))
	float RespawnTime = 4.0f;

	/** 用于重生拾取物的计时器 */
	FTimerHandle RespawnTimer;

public:	
	
	/** 构造函数 */
	AShooterPickup();

protected:

	/** 原生构造脚本 */
	virtual void OnConstruction(const FTransform& Transform) override;

	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 游戏逻辑清理 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 处理碰撞重叠 */
	UFUNCTION()
	virtual void OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

protected:

	/** 当拾取物需要重生时调用 */
	void RespawnPickup();

	/** 将控制权交给蓝图以播放拾取物重生动画。应在结束时调用 FinishRespawn */
	UFUNCTION(BlueprintImplementableEvent, Category="Pickup", meta = (DisplayName = "OnRespawn"))
	void BP_OnRespawn();

	/** 重生后启用该拾取物 */
	UFUNCTION(BlueprintCallable, Category="Pickup")
	void FinishRespawn();
};
