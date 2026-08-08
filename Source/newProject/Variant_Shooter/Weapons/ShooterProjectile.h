// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class ACharacter;
class UPrimitiveComponent;

/**
 *  第一人称射击游戏的简单抛射物类
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterProjectile : public AActor
{
	GENERATED_BODY()
	
	/** 为抛射物提供碰撞检测 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USphereComponent* CollisionComponent;

	/** 处理抛射物的移动 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UProjectileMovementComponent* ProjectileMovement;

protected:

	/** 该抛射物命中时产生的 AI 感知噪音响度 */
	UPROPERTY(EditAnywhere, Category="Projectile|Noise", meta = (ClampMin = 0, ClampMax = 100))
	float NoiseLoudness = 3.0f;

	/** 该抛射物命中时产生的 AI 感知噪音范围 */
	UPROPERTY(EditAnywhere, Category="Projectile|Noise", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float NoiseRange = 3000.0f;

	/** 该抛射物命中时产生的 AI 感知噪音标签 */
	UPROPERTY(EditAnywhere, Category="Noise")
	FName NoiseTag = FName("Projectile");

	/** 命中时施加的物理力 */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit", meta = (ClampMin = 0, ClampMax = 50000))
	float PhysicsForce = 100.0f;

	/** 命中时造成的伤害 */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit", meta = (ClampMin = 0, ClampMax = 100))
	float HitDamage = 25.0f;

	/** 施加的伤害类型。可用于表示特定伤害类型，如火焰、爆炸等 */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit")
	TSubclassOf<UDamageType> HitDamageType;

	/** 如果为 true，抛射物可伤害发射它的角色 */
	UPROPERTY(EditAnywhere, Category="Projectile|Hit")
	bool bDamageOwner = false;

	/** 如果为 true，抛射物将爆炸，对范围内所有 Actor 施加径向伤害 */
	UPROPERTY(EditAnywhere, Category="Projectile|Explosion")
	bool bExplodeOnHit = false;

	/** 受爆炸伤害影响的 Actor 的最大距离 */
	UPROPERTY(EditAnywhere, Category="Projectile|Explosion", meta = (ClampMin = 0, ClampMax = 5000, Units = "cm"))
	float ExplosionRadius = 500.0f;	

	/** 如果为 true，该抛射物已命中其他表面 */
	bool bHit = false;

	/** 命中后等待多长时间再销毁该抛射物 */
	UPROPERTY(EditAnywhere, Category="Projectile|Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float DeferredDestructionTime = 5.0f;

	/** 处理该抛射物延迟销毁的计时器 */
	FTimerHandle DestructionTimer;

public:	

	/** 构造函数 */
	AShooterProjectile();

protected:
	
	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 游戏逻辑清理 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 处理碰撞 */
	virtual void NotifyHit(class UPrimitiveComponent* MyComp, AActor* Other, UPrimitiveComponent* OtherComp, bool bSelfMoved, FVector HitLocation, FVector HitNormal, FVector NormalImpulse, const FHitResult& Hit) override;

protected:

	/** 查找爆炸半径内的 Actor 并对其造成伤害 */
	void ExplosionCheck(const FVector& ExplosionCenter);

	/** 处理指定 Actor 受到的抛射物命中 */
	void ProcessHit(AActor* HitActor, UPrimitiveComponent* HitComp, const FVector& HitLocation, const FVector& HitDirection);

	/** 将控制权交给蓝图以实现命中时的各种效果。 */
	UFUNCTION(BlueprintImplementableEvent, Category="Projectile", meta = (DisplayName = "On Projectile Hit"))
	void BP_OnProjectileHit(const FHitResult& Hit);

	/** 由销毁计时器调用以销毁该抛射物 */
	void OnDeferredDestruction();

};
