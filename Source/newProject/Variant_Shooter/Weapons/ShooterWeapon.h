// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "ShooterWeapon.generated.h"

class IShooterWeaponHolder;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;

/**
 *  简单第一人称射击武器的基类
 *  提供第一人称和第三人称视角的网格
 *  处理弹药与开火逻辑
 *  通过 ShooterWeaponHolder 接口与武器持有者交互
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterWeapon : public AActor
{
	GENERATED_BODY()

	/** 第一人称视角网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** 第三人称视角网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

protected:

	/** 指向武器持有者的转换指针 */
	IShooterWeaponHolder* WeaponOwner;

	/** 开火时生成的视觉弹道 Actor 类 */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AActor> TracerClass;

	/** 每次命中造成的伤害 */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 500))
	float HitDamage = 25.0f;

	/** 每个弹匣的子弹数量 */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** 携带的备用弹匣数量 */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 20))
	int32 MaxReserveMagazines = 5;

	/** 当前弹匣中的子弹数量 */
	int32 CurrentBullets = 0;

	/** 剩余备用弹匣数量 */
	int32 ReserveMagazines = 1;

	/** 临时增益的伤害倍率 */
	float DamageMultiplier = 1.0f;

	/** 开火该武器时播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** 装备该武器时在角色上播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* EquipMontage;

	/** 卸下该武器时在角色上播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* UnequipMontage;

	/** 为该武器换弹时在角色上播放的动画蒙太奇 */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* ReloadMontage;

	/** 该武器激活时为第一人称角色网格设置的 AnimInstance 类 */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** 该武器激活时为第三人称角色网格设置的 AnimInstance 类 */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** 瞄准时的散布锥半角 */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** 施加给持有者的开火后坐力大小 */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 1.8f;

	// ---- 后坐力 ----

	/** 每次射击时武器网格向后推动的距离 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0, ClampMax = 50, Units = "cm"))
	float WeaponKickBack = 0.0f;

	/** 每次射击时武器网格向上抬起的距离 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0, ClampMax = 50, Units = "cm"))
	float WeaponKickUp = 0.0f;

	/** 武器冲击恢复到静止位置所需的时间 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.01f, ClampMax = 1.0f, Units = "s"))
	float WeaponKickDuration = 0.12f;

	/** 当前归一化的武器冲击量（1 = 完全冲击，衰减到 0） */
	float WeaponKickAlpha = 0.0f;

	/** 武器网格的静止变换，挂接时捕获，供冲击恢复使用 */
	FTransform FirstPersonMeshRestTransform;
	FTransform ThirdPersonMeshRestTransform;
	bool bRestTransformsCaptured = false;

	/** 第一人称枪口插槽名称，抛射物将从这里生成 */
	UPROPERTY(EditAnywhere, Category="Aim")
	FName MuzzleSocketName;

	/** 子弹在枪口前方的生成距离 */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	/** 即时命中开火的最大距离 */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxFireRange = 10000.0f;

	/** 如果为 true，该武器将以射速自动开火 */
	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	/** 该武器两次射击之间的时间。同时影响全自动和半自动模式 */
	UPROPERTY(EditAnywhere, Category="Refire", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.5f;

	/** 换弹所需时间（未设置换弹动画时作为回退值） */
	UPROPERTY(EditAnywhere, Category="Reload", meta = (ClampMin = 0.1f, ClampMax = 10, Units = "s"))
	float ReloadTime = 2.0f;

	/** 上一次射击的游戏时间，用于在半自动模式下强制执行射速 */
	float TimeOfLastShot = 0.0f;

	/** 如果为 true，该武器正在开火 */
	bool bIsFiring = false;

	/** 如果为 true，该武器正在换弹 */
	bool bIsReloading = false;

	/** 客户端为单次射击传入的覆盖目标 */
	FVector OverrideAimTarget = FVector::ZeroVector;

	/** 处理全自动连射的计时器 */
	FTimerHandle RefireTimer;

	/** 处理换弹完成的计时器 */
	FTimerHandle ReloadTimer;

	/** 指向持有者的转换 Pawn 指针，用于 AI 感知系统交互 */
	TObjectPtr<APawn> PawnOwner;

	/** 供 AI 感知系统交互使用的枪声响度 */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** 枪声 AI 感知噪音的最大范围 */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 3000.0f;

	/** 应用于射击该武器产生的噪音的标签 */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName ShotNoiseTag = FName("Shot");

public:

	/** 构造函数 */
	AShooterWeapon();

protected:

	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 游戏逻辑清理 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** 当武器的持有者被销毁时调用 */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

public:

	/** 激活该武器并使其做好开火准备 */
	void ActivateWeapon();

	/** 停用该武器 */
	void DeactivateWeapon();

	/** 开始射击该武器，可选择覆盖目标 */
	void StartFiring(const FVector& OverrideTarget = FVector::ZeroVector);

	/** 停止射击该武器 */
	void StopFiring();

	/** 开始为该武器换弹 */
	void Reload();

	/** 如果武器可以换弹（未满、未在换弹、有备用弹药）则返回 true */
	bool CanReload() const { return CurrentBullets < MagazineSize && !bIsReloading && ReserveMagazines > 0; }

	/** 如果武器正在换弹则返回 true */
	bool IsReloading() const { return bIsReloading; }

	/** 如果武器正在开火则返回 true */
	bool IsFiring() const { return bIsFiring; }

	/** 如果该武器是全自动则返回 true */
	bool IsFullAuto() const { return bFullAuto; }

	/** 返回射速 */
	float GetRefireRate() const { return RefireRate; }

	/** 更新覆盖瞄准目标（用于全自动射击期间客户端的瞄准流） */
	void UpdateOverrideAimTarget(const FVector& Target) { OverrideAimTarget = Target; }

protected:

	/** 开火 */
	virtual void Fire();

	/** 每帧调用以将武器后坐力冲击动画恢复到静止位置 */
	virtual void Tick(float DeltaSeconds) override;

	/** 半自动武器射击时射速时间结束后调用 */
	void FireCooldownExpired();

	/** 向目标位置开火 */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** 使用本地观看者可见的网格返回枪口世界位置 */
	FVector GetMuzzleLocation() const;

	/** 为一次射击向后/向上推动武器网格 */
	void TriggerWeaponKick();

	/** 将当前冲击量应用到两个武器网格 */
	void ApplyWeaponKick();

	/** 换弹计时器到期时调用——装满弹匣 */
	void FinishReload();

public:

	/** 返回第一人称网格 */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** 返回第三人称网格 */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** 返回第一人称动画实例类 */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** 返回第三人称动画实例类 */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** 返回弹匣容量 */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** 返回当前子弹数量 */
	int32 GetBulletCount() const { return CurrentBullets; }

	/** 返回备用弹匣数量 */
	int32 GetReserveMagazines() const { return ReserveMagazines; }

	/** 返回最大备用弹匣数量 */
	int32 GetMaxReserveMagazines() const { return MaxReserveMagazines; }

	/** 增加备用弹匣（来自击杀奖励） */
	void AddReserveMagazines(int32 Count);

	/** 设置临时伤害倍率 */
	void SetDamageMultiplier(float Multiplier) { DamageMultiplier = Multiplier; }

	/** 返回伤害倍率 */
	float GetDamageMultiplier() const { return DamageMultiplier; }

	/** 返回装备动画蒙太奇 */
	UAnimMontage* GetEquipMontage() const { return EquipMontage; }

	/** 返回卸下武器动画蒙太奇 */
	UAnimMontage* GetUnequipMontage() const { return UnequipMontage; }

	/** 返回开火蒙太奇 */
	UAnimMontage* GetFiringMontage() const { return FiringMontage; }

	/** 返回换弹蒙太奇 */
	UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	/** 返回开火后坐力大小 */
	float GetFiringRecoil() const { return FiringRecoil; }

	/** 让持有者在网格上播放装备蒙太奇 */
	void PlayEquipMontage();

	/** 让持有者在网格上播放卸下武器蒙太奇 */
	void PlayUnequipMontage();

	/** 向所有客户端多播开火效果 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnFireEffects(int32 BulletCount, const FVector& ImpactPoint);

	/** 向所有客户端多播换弹效果 */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnReloadEffects();
};
