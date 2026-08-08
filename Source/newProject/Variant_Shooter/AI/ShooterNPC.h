// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "newProjectCharacter.h"
#include "ShooterWeaponHolder.h"
#include "ShooterNPC.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FPawnDeathDelegate);

class AShooterWeapon;

/**
 *  一个由 AI 控制的简单射击游戏 NPC
 *  通过其 AI 控制器管理的 StateTree 执行行为
 *  持有并管理一件武器
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterNPC : public AnewProjectCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()

public:

	/** 该角色当前的 HP。受到伤害降为零时死亡 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category="Damage")
	float CurrentHP = 100.0f;

protected:

	/** 布娃娃死亡时使用的碰撞配置文件名称 */
	UPROPERTY(EditAnywhere, Category="Damage")
	FName RagdollCollisionProfile = FName("Ragdoll");

	/** 死亡后等待多长时间再销毁此 Actor */
	UPROPERTY(EditAnywhere, Category="Damage")
	float DeferredDestructionTime = 5.0f;

	/** 该角色的队伍字节 */
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 1;

	/** 该角色死亡时添加的 Actor 标签 */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** 指向已装备武器的指针 */
	UPROPERTY(Replicated)
	TObjectPtr<AShooterWeapon> Weapon;

	/** 为该角色生成的武器类型 */
	UPROPERTY(EditAnywhere, Category="Weapon")
	TSubclassOf<AShooterWeapon> WeaponClass;

	/** 第一人称网格的武器插槽名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** 第三人称网格的武器插槽名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** 瞄准计算的最大距离 */
	UPROPERTY(EditAnywhere, Category="Aim")
	float AimRange = 10000.0f;

	/** 瞄准时应用的锥形散布角度 */
	UPROPERTY(EditAnywhere, Category="Aim")
	float AimVarianceHalfAngle = 10.0f;

	/** 瞄准时相对目标中心的最小垂直偏移 */
	UPROPERTY(EditAnywhere, Category="Aim")
	float MinAimOffsetZ = -35.0f;

	/** 瞄准时相对目标中心的最大垂直偏移 */
	UPROPERTY(EditAnywhere, Category="Aim")
	float MaxAimOffsetZ = -60.0f;

	/** 当前正在瞄准的 Actor */
	TObjectPtr<AActor> CurrentAimTarget;

	/** 如果为 true，该角色正在射击其武器 */
	bool bIsShooting = false;

	/** 如果为 true，该角色已经死亡 */
	UPROPERTY(ReplicatedUsing = OnRep_bIsDead)
	bool bIsDead = false;

	/** 最后伤害该 NPC 的控制器（用于击杀记录） */
	AController* LastDamageInstigator = nullptr;

	/** 死亡后延迟销毁的计时器 */
	FTimerHandle DeathTimer;

public:

	/** 该 NPC 死亡时调用的委托 */
	FPawnDeathDelegate OnPawnDeath;

	/** 当 bIsDead 复制到客户端时调用 */
	UFUNCTION()
	void OnRep_bIsDead();

protected:

	/** 属性复制 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 游戏逻辑清理 */
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	/** 处理受到的伤害 */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	//~Begin IShooterWeaponHolder interface

	/** 将武器网格挂接到持有者 */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;

	/** 播放武器的开火蒙太奇 */
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;

	/** 向持有者应用武器后坐力 */
	virtual void AddWeaponRecoil(float Recoil) override;

	/** 用当前弹药数量更新武器 HUD */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;

	/** 计算并返回武器的瞄准位置 */
	virtual FVector GetWeaponTargetLocation() override;

	/** 向持有者给予此类武器 */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;

	/** 激活传入的武器 */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;

	/** 停用传入的武器 */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;

	/** 通知持有者武器冷却已结束，可以再次射击 */
	virtual void OnSemiWeaponRefire() override;

	/** 在角色网格上播放装备动画 */
	virtual void PlayEquipMontage(UAnimMontage* Montage) override;

	/** 在角色网格上播放卸下武器动画 */
	virtual void PlayUnequipMontage(UAnimMontage* Montage) override;

	/** 在角色网格上播放换弹动画 */
	virtual void PlayReloadMontage(UAnimMontage* Montage) override;

	//~End IShooterWeaponHolder interface

protected:

	/** 当 HP 耗尽且角色应死亡时调用 */
	void Die();

	/** 死亡后调用以销毁 Actor */
	void DeferredDestruction();

public:

	/** 通知该角色开始向传入的 Actor 射击 */
	void StartShooting(AActor* ActorToShoot);

	/** 通知该角色停止射击 */
	void StopShooting();

	/** 如果 NPC 已死亡则返回 true */
	bool IsDead() const { return bIsDead; }
};
