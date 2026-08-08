// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "newProjectCharacter.h"
#include "ShooterWeaponHolder.h"
#include "ShooterCharacter.generated.h"

class AShooterWeapon;
class UInputAction;
class UInputComponent;
class UPawnNoiseEmitterComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBulletCountUpdatedDelegate, int32, MagazineSize, int32, Bullets);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FDamagedDelegate, float, LifePercent);

UCLASS(abstract)
class NEWPROJECT_API AShooterCharacter : public AnewProjectCharacter, public IShooterWeaponHolder
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UPawnNoiseEmitterComponent* PawnNoiseEmitter;

protected:

	/** 开火输入操作 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** 换弹输入操作 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ReloadAction;

	/** 切换武器输入操作 */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** 第一人称网格的武器插槽名称 */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** 第三人称网格的武器插槽名称 */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** 瞄准射线使用的最大距离 */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** 该角色可拥有的最大 HP */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** 该角色当前剩余 HP */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP)
	float CurrentHP = 0.0f;

	/** 该角色的队伍 ID */
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** 该角色死亡时添加的 Actor 标签 */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** 角色初始拥有的武器类 */
	UPROPERTY(EditAnywhere, Category="Weapons")
	TArray<TSubclassOf<AShooterWeapon>> DefaultWeapons;

	/** 角色获得的武器列表 */
	TArray<AShooterWeapon*> OwnedWeapons;

	/** 当前装备并可射击的武器 */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	/** 武器切换动画的持续时间 */
	UPROPERTY(EditAnywhere, Category="Weapons", meta = (ClampMin = 0.0f, ClampMax = 2.0f, Units = "s"))
	float WeaponSwitchDuration = 0.4f;

	/** 武器切换动画播放期间为 true */
	bool bIsSwitchingWeapon = false;

	/** 动画结束后完成武器切换的计时器句柄 */
	FTimerHandle WeaponSwitchTimerHandle;

	/** 待切换武器的索引 */
	int32 PendingWeaponIndex = 0;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

	/** 全自动射击期间持续向服务器发送瞄准更新的计时器 */
	FTimerHandle AimUpdateTimer;

	// ---- 后坐力 ----

	/** 开火后相机回到原始瞄准位置的速度 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.1f, ClampMax = 60.0f, Units = "deg/s"))
	float RecoilRecoverySpeed = 12.0f;

	/** 开火时相机累计的最大俯仰冲击 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.1f, ClampMax = 45.0f, Units = "deg"))
	float MaxRecoilPitch = 3.0f;

	/** 开火时相机累计的最大偏航冲击 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.1f, ClampMax = 45.0f, Units = "deg"))
	float MaxRecoilYaw = 1.5f;

	/** 相机获得的水平冲击相对垂直冲击的比例 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.0f, ClampMax = 1.0f))
	float RecoilYawRatio = 0.35f;

	/** 如果为 true，在第一人称手臂上播放武器开火动画（可能导致枪口下沉/抖动） */
	UPROPERTY(EditAnywhere, Category="Recoil")
	bool bPlayFiringMontage = false;

	/** 单次射击施加的最大俯仰冲击 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.0f, ClampMax = 10.0f, Units = "deg"))
	float MaxRecoilPitchPerShot = 1.5f;

	/** 最后一发子弹射出后，相机开始回到原始瞄准位置前的延迟 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.0f, ClampMax = 2.0f, Units = "s"))
	float RecoilSettleDelay = 0.15f;

	/** 当装备的武器未配置开火后坐力时使用的相机冲击 */
	UPROPERTY(EditAnywhere, Category="Recoil", meta = (ClampMin = 0.0f, ClampMax = 30.0f, Units = "deg"))
	float DefaultWeaponRecoil = 1.2f;

	/** 当前等待恢复的相机后坐力偏移 */
	float CurrentRecoilPitch = 0.0f;
	float CurrentRecoilYaw = 0.0f;

	/** 让相机平滑回到射击前瞄准位置的计时器 */
	FTimerHandle RecoilRecoveryTimer;
	FTimerHandle RecoilSettleTimer;

	/** 停止射击后启动恢复计时器 */
	void StartRecoilRecovery();

	/** 让相机逐步回到射击前瞄准位置 */
	void UpdateRecoilRecovery();

	// ---- 增益系统 ----

	/** 击杀时恢复生命的概率 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 1))
	float HealthRestoreChance = 0.3f;

	/** 恢复的生命值 */
	UPROPERTY(EditAnywhere, Category="Buffs")
	float HealthRestoreAmount = 50.0f;

	/** 击杀时获得伤害增益的概率 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 1))
	float DamageBoostChance = 0.3f;

	/** 增益期间的伤害倍率 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 1, ClampMax = 10))
	float DamageBoostMultiplier = 2.0f;

	/** 伤害增益持续时间 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 60))
	float DamageBoostDuration = 10.0f;

	/** 击杀时获得分数增益的概率 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 1))
	float ScoreBoostChance = 0.2f;

	/** 增益期间的分数倍率 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 1, ClampMax = 10))
	float ScoreBoostMultiplier = 2.0f;

	/** 分数增益持续时间 */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 60))
	float ScoreBoostDuration = 15.0f;

	/** 最后伤害该角色的控制器 */
	AController* LastDamageInstigator = nullptr;

	/** 伤害增益是否生效 */
	bool bHasDamageBoost = false;

	/** 分数增益是否生效 */
	bool bHasScoreBoost = false;

	FTimerHandle DamageBoostTimer;
	FTimerHandle ScoreBoostTimer;

	void ActivateDamageBoost();
	void DeactivateDamageBoost();
	void ActivateScoreBoost();
	void DeactivateScoreBoost();

	/** 仅向拥有该角色的客户端发送增益通知 */
	void ShowBuffMessage(const FString& Message, FColor Color);

public:

	/** 子弹数量更新委托 */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** 受击委托 */
	FDamagedDelegate OnDamaged;

public:

	/** 构造函数 */
	AShooterCharacter();

protected:

	/** 属性复制 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 当 CurrentHP 复制到客户端时调用 */
	UFUNCTION()
	void OnRep_CurrentHP();

	/** 当 CurrentWeapon 复制到客户端时调用 */
	UFUNCTION()
	void OnRep_CurrentWeapon();

	/** 游戏逻辑初始化 */
	virtual void BeginPlay() override;

	/** 游戏逻辑清理 */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** 设置输入操作绑定 */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** 处理受到的伤害 */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** 处理来自控制或 UI 界面的瞄准输入 */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** 处理来自控制或 UI 界面的移动输入 */
	virtual void DoMove(float Right, float Forward)  override;

	/** 处理来自控制或 UI 界面的跳跃开始输入 */
	virtual void DoJumpStart()  override;

	/** 处理来自控制或 UI 界面的跳跃结束输入 */
	virtual void DoJumpEnd()  override;

	/** 处理开始开火输入 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** 处理停止开火输入 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** 由计时器调用，向服务器持续发送瞄准目标更新 */
	void StreamAimTarget();

	/** 开始开火的服务器 RPC */
	UFUNCTION(Server, Reliable)
	void Server_StartFiring(FVector ClientAimTarget);

	/** 停止开火的服务器 RPC */
	UFUNCTION(Server, Reliable)
	void Server_StopFiring();

	/** 全自动射击期间更新瞄准目标的服务器 RPC */
	UFUNCTION(Server, Unreliable)
	void Server_UpdateAimTarget(FVector ClientAimTarget);

	/** 处理切换武器输入 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** 切换武器的服务器 RPC */
	UFUNCTION(Server, Reliable)
	void Server_SwitchWeapon();

	/** 处理换弹输入 */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoReload();

	/** 换弹的服务器 RPC */
	UFUNCTION(Server, Reliable)
	void Server_Reload();

	/** 播放装备动画的多播 RPC */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEquipMontage(UAnimMontage* Montage);

	/** 播放卸下武器动画的多播 RPC */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayUnequipMontage(UAnimMontage* Montage);

public:

	//~Begin IShooterWeaponHolder interface

	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) override;
	virtual void PlayFiringMontage(UAnimMontage* Montage) override;
	virtual void AddWeaponRecoil(float Recoil) override;
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) override;
	virtual FVector GetWeaponTargetLocation() override;
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) override;
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) override;
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) override;
	virtual void OnSemiWeaponRefire() override;
	virtual void PlayEquipMontage(UAnimMontage* Montage) override;
	virtual void PlayUnequipMontage(UAnimMontage* Montage) override;
	virtual void PlayReloadMontage(UAnimMontage* Montage) override;

	//~End IShooterWeaponHolder interface

protected:

	/** 如果角色已拥有指定类的武器则返回 true */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** 内部武器切换逻辑（仅在服务器运行） */
	void SwitchWeapon_Internal();

	/** 切换实际武器和 ABP，然后播放装备动画 */
	void PerformWeaponSwap();

	/** 装备动画结束后调用以完成切换 */
	void FinishWeaponSwitch();

	/** 当该角色 HP 耗尽时调用 */
	void Die();

	/** 在所有客户端播放死亡效果的多播 RPC */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	/** 允许蓝图代码响应该角色死亡的调用 */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** 由重生计时器调用，销毁该角色并强制玩家控制器重生 */
	void OnRespawn();

public:

	/** 如果角色已死亡则返回 true */
	bool IsDead() const;

	/** 返回当前 HP */
	float GetCurrentHP() const { return CurrentHP; }

	/** 返回最大 HP */
	float GetMaxHP() const { return MaxHP; }

	/** 发放击杀奖励：弹药 + 随机增益 */
	void GrantKillRewards();

	/** 返回用于计分的分数倍率 */
	float GetScoreMultiplier() const { return bHasScoreBoost ? ScoreBoostMultiplier : 1.0f; }
};
