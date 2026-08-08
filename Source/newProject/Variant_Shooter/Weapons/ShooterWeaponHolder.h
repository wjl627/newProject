// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ShooterWeaponHolder.generated.h"

class AShooterWeapon;
class UAnimMontage;


UINTERFACE(MinimalAPI)
class UShooterWeaponHolder : public UInterface
{
	GENERATED_BODY()
};

/**
 *  Shooter 游戏武器持有者类的通用接口
 */
class NEWPROJECT_API IShooterWeaponHolder
{
	GENERATED_BODY()

public:

	/** 将武器网格挂接到持有者 */
	virtual void AttachWeaponMeshes(AShooterWeapon* Weapon) = 0;

	/** 播放武器的开火蒙太奇 */
	virtual void PlayFiringMontage(UAnimMontage* Montage) = 0;

	/** 向持有者应用武器后坐力 */
	virtual void AddWeaponRecoil(float Recoil) = 0;

	/** 用当前弹药数量更新武器 HUD */
	virtual void UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize) = 0;

	/** 计算并返回武器的瞄准位置 */
	virtual FVector GetWeaponTargetLocation() = 0;

	/** 向持有者给予此类武器 */
	virtual void AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass) = 0;

	/** 激活传入的武器 */
	virtual void OnWeaponActivated(AShooterWeapon* Weapon) = 0;

	/** 停用传入的武器 */
	virtual void OnWeaponDeactivated(AShooterWeapon* Weapon) = 0;

	/** 通知持有者武器冷却已结束，可以再次射击 */
	virtual void OnSemiWeaponRefire() = 0;

	/** 在角色网格上播放装备蒙太奇 */
	virtual void PlayEquipMontage(UAnimMontage* Montage) = 0;

	/** 在角色网格上播放卸下武器蒙太奇 */
	virtual void PlayUnequipMontage(UAnimMontage* Montage) = 0;

	/** 在角色网格上播放换弹蒙太奇 */
	virtual void PlayReloadMontage(UAnimMontage* Montage) = 0;
};
