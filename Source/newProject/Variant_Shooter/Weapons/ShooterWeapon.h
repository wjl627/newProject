// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterWeaponHolder.h"
#include "Animation/AnimInstance.h"
#include "ShooterWeapon.generated.h"

class IShooterWeaponHolder;
class AShooterProjectile;
class USkeletalMeshComponent;
class UAnimMontage;
class UAnimInstance;

/**
 *  Base class for a simple first person shooter weapon
 *  Provides both first person and third person perspective meshes
 *  Handles ammo and firing logic
 *  Interacts with the weapon owner through the ShooterWeaponHolder interface
 */
UCLASS(abstract)
class NEWPROJECT_API AShooterWeapon : public AActor
{
	GENERATED_BODY()

	/** First person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonMesh;

	/** Third person perspective mesh */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* ThirdPersonMesh;

protected:

	/** Cast pointer to the weapon owner */
	IShooterWeaponHolder* WeaponOwner;

	/** Type of projectiles this weapon will shoot (deprecated — use hitscan) */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AShooterProjectile> ProjectileClass;

	/** Visual tracer actor class spawned on fire */
	UPROPERTY(EditAnywhere, Category="Ammo")
	TSubclassOf<AActor> TracerClass;

	/** Damage dealt per hit */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 500))
	float HitDamage = 25.0f;

	/** Number of bullets in a magazine */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 100))
	int32 MagazineSize = 10;

	/** Number of reserve magazines carried */
	UPROPERTY(EditAnywhere, Category="Ammo", meta = (ClampMin = 0, ClampMax = 20))
	int32 MaxReserveMagazines = 5;

	/** Number of bullets in the current magazine */
	int32 CurrentBullets = 0;

	/** Number of reserve magazines remaining */
	int32 ReserveMagazines = 1;

	/** Damage multiplier for temporary buffs */
	float DamageMultiplier = 1.0f;

	/** Animation montage to play when firing this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* FiringMontage;

	/** Animation montage to play on the character when equipping this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* EquipMontage;

	/** Animation montage to play on the character when unequipping this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* UnequipMontage;

	/** Animation montage to play on the character when reloading this weapon */
	UPROPERTY(EditAnywhere, Category="Animation")
	UAnimMontage* ReloadMontage;

	/** AnimInstance class to set for the first person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> FirstPersonAnimInstanceClass;

	/** AnimInstance class to set for the third person character mesh when this weapon is active */
	UPROPERTY(EditAnywhere, Category="Animation")
	TSubclassOf<UAnimInstance> ThirdPersonAnimInstanceClass;

	/** Cone half-angle for variance while aiming */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 90, Units = "Degrees"))
	float AimVariance = 0.0f;

	/** Amount of firing recoil to apply to the owner */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100))
	float FiringRecoil = 0.0f;

	/** Name of the first person muzzle socket where projectiles will spawn */
	UPROPERTY(EditAnywhere, Category="Aim")
	FName MuzzleSocketName;

	/** Distance ahead of the muzzle that bullets will spawn at */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
	float MuzzleOffset = 10.0f;

	/** Max distance for hitscan fire */
	UPROPERTY(EditAnywhere, Category="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxFireRange = 10000.0f;

	/** If true, this weapon will automatically fire at the refire rate */
	UPROPERTY(EditAnywhere, Category="Refire")
	bool bFullAuto = false;

	/** Time between shots for this weapon. Affects both full auto and semi auto modes */
	UPROPERTY(EditAnywhere, Category="Refire", meta = (ClampMin = 0, ClampMax = 5, Units = "s"))
	float RefireRate = 0.5f;

	/** Time it takes to reload (used as fallback if no ReloadMontage is set) */
	UPROPERTY(EditAnywhere, Category="Reload", meta = (ClampMin = 0.1f, ClampMax = 10, Units = "s"))
	float ReloadTime = 2.0f;

	/** Game time of last shot fired, used to enforce refire rate on semi auto */
	float TimeOfLastShot = 0.0f;

	/** If true, the weapon is currently firing */
	bool bIsFiring = false;

	/** If true, the weapon is currently reloading */
	bool bIsReloading = false;

	/** Override target passed from client for one shot */
	FVector OverrideAimTarget = FVector::ZeroVector;

	/** Timer to handle full auto refiring */
	FTimerHandle RefireTimer;

	/** Timer to handle reload completion */
	FTimerHandle ReloadTimer;

	/** Cast pawn pointer to the owner for AI perception system interactions */
	TObjectPtr<APawn> PawnOwner;

	/** Loudness of the shot for AI perception system interactions */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100))
	float ShotLoudness = 1.0f;

	/** Max range of shot AI perception noise */
	UPROPERTY(EditAnywhere, Category="Perception", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float ShotNoiseRange = 3000.0f;

	/** Tag to apply to noise generated by shooting this weapon */
	UPROPERTY(EditAnywhere, Category="Perception")
	FName ShotNoiseTag = FName("Shot");

public:

	/** Constructor */
	AShooterWeapon();

protected:

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay Cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

protected:

	/** Called when the weapon's owner is destroyed */
	UFUNCTION()
	void OnOwnerDestroyed(AActor* DestroyedActor);

public:

	/** Activates this weapon and gets it ready to fire */
	void ActivateWeapon();

	/** Deactivates this weapon */
	void DeactivateWeapon();

	/** Start firing this weapon, optionally overriding the target */
	void StartFiring(const FVector& OverrideTarget = FVector::ZeroVector);

	/** Stop firing this weapon */
	void StopFiring();

	/** Start reloading this weapon */
	void Reload();

	/** Returns true if the weapon can be reloaded (not full, not already reloading, has reserve) */
	bool CanReload() const { return CurrentBullets < MagazineSize && !bIsReloading && ReserveMagazines > 0; }

	/** Returns true if the weapon is currently reloading */
	bool IsReloading() const { return bIsReloading; }

	/** Returns true if the weapon is currently firing */
	bool IsFiring() const { return bIsFiring; }

	/** Returns true if this weapon is full-auto */
	bool IsFullAuto() const { return bFullAuto; }

	/** Returns the refire rate */
	float GetRefireRate() const { return RefireRate; }

	/** Updates the override aim target (for client aim streaming during full-auto) */
	void UpdateOverrideAimTarget(const FVector& Target) { OverrideAimTarget = Target; }

protected:

	/** Fire the weapon */
	virtual void Fire();

	/** Called when the refire rate time has passed while shooting semi auto weapons */
	void FireCooldownExpired();

	/** Fire a projectile towards the target location */
	virtual void FireProjectile(const FVector& TargetLocation);

	/** Calculates the spawn transform for projectiles shot by this weapon */
	FTransform CalculateProjectileSpawnTransform(const FVector& TargetLocation) const;

	/** Called when the reload timer expires — refills the magazine */
	void FinishReload();

public:

	/** Returns the first person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; };

	/** Returns the third person mesh */
	UFUNCTION(BlueprintPure, Category="Weapon")
	USkeletalMeshComponent* GetThirdPersonMesh() const { return ThirdPersonMesh; };

	/** Returns the first person anim instance class */
	const TSubclassOf<UAnimInstance>& GetFirstPersonAnimInstanceClass() const;

	/** Returns the third person anim instance class */
	const TSubclassOf<UAnimInstance>& GetThirdPersonAnimInstanceClass() const;

	/** Returns the magazine size */
	int32 GetMagazineSize() const { return MagazineSize; };

	/** Returns the current bullet count */
	int32 GetBulletCount() const { return CurrentBullets; }

	/** Returns the number of reserve magazines */
	int32 GetReserveMagazines() const { return ReserveMagazines; }

	/** Returns the max reserve magazines */
	int32 GetMaxReserveMagazines() const { return MaxReserveMagazines; }

	/** Adds reserve magazines (from kill rewards) */
	void AddReserveMagazines(int32 Count);

	/** Sets a temporary damage multiplier */
	void SetDamageMultiplier(float Multiplier) { DamageMultiplier = Multiplier; }

	/** Returns the damage multiplier */
	float GetDamageMultiplier() const { return DamageMultiplier; }

	/** Returns the equip animation montage */
	UAnimMontage* GetEquipMontage() const { return EquipMontage; }

	/** Returns the unequip animation montage */
	UAnimMontage* GetUnequipMontage() const { return UnequipMontage; }

	/** Returns the firing montage */
	UAnimMontage* GetFiringMontage() const { return FiringMontage; }

	/** Returns the reload montage */
	UAnimMontage* GetReloadMontage() const { return ReloadMontage; }

	/** Returns the firing recoil amount */
	float GetFiringRecoil() const { return FiringRecoil; }

	/** Make the owner play the equip montage on its mesh */
	void PlayEquipMontage();

	/** Make the owner play the unequip montage on its mesh */
	void PlayUnequipMontage();

	/** Multicast firing effects to all clients */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnFireEffects(int32 BulletCount, const FVector& MuzzleLocation, const FVector& ImpactPoint);

	/** Multicast reload effects to all clients */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnReloadEffects();
};
