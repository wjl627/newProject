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

	/** Fire weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* FireAction;

	/** Reload weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* ReloadAction;

	/** Switch weapon input action */
	UPROPERTY(EditAnywhere, Category ="Input")
	UInputAction* SwitchWeaponAction;

	/** Name of the first person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName FirstPersonWeaponSocket = FName("HandGrip_R");

	/** Name of the third person mesh weapon socket */
	UPROPERTY(EditAnywhere, Category ="Weapons")
	FName ThirdPersonWeaponSocket = FName("HandGrip_R");

	/** Max distance to use for aim traces */
	UPROPERTY(EditAnywhere, Category ="Aim", meta = (ClampMin = 0, ClampMax = 100000, Units = "cm"))
	float MaxAimDistance = 10000.0f;

	/** Max HP this character can have */
	UPROPERTY(EditAnywhere, Category="Health")
	float MaxHP = 500.0f;

	/** Current HP remaining to this character */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentHP)
	float CurrentHP = 0.0f;

	/** Team ID for this character*/
	UPROPERTY(EditAnywhere, Category="Team")
	uint8 TeamByte = 0;

	/** Actor tag to grant this character when it dies */
	UPROPERTY(EditAnywhere, Category="Team")
	FName DeathTag = FName("Dead");

	/** Weapon classes the character starts with */
	UPROPERTY(EditAnywhere, Category="Weapons")
	TArray<TSubclassOf<AShooterWeapon>> DefaultWeapons;

	/** List of weapons picked up by the character */
	TArray<AShooterWeapon*> OwnedWeapons;

	/** Weapon currently equipped and ready to shoot with */
	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	TObjectPtr<AShooterWeapon> CurrentWeapon;

	/** Duration of the weapon switch animation */
	UPROPERTY(EditAnywhere, Category="Weapons", meta = (ClampMin = 0.0f, ClampMax = 2.0f, Units = "s"))
	float WeaponSwitchDuration = 0.4f;

	/** True while a weapon switch animation is playing */
	bool bIsSwitchingWeapon = false;

	/** Timer handle for completing weapon switch after animation */
	FTimerHandle WeaponSwitchTimerHandle;

	/** Index of the pending weapon to switch to */
	int32 PendingWeaponIndex = 0;

	UPROPERTY(EditAnywhere, Category ="Destruction", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float RespawnTime = 5.0f;

	FTimerHandle RespawnTimer;

	/** Timer for continuously sending aim updates to server during full-auto fire */
	FTimerHandle AimUpdateTimer;

	// ---- Buff System ----

	/** Chance to restore health on kill */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 1))
	float HealthRestoreChance = 0.3f;

	/** Amount of health restored */
	UPROPERTY(EditAnywhere, Category="Buffs")
	float HealthRestoreAmount = 50.0f;

	/** Chance to get damage boost on kill */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 1))
	float DamageBoostChance = 0.3f;

	/** Damage multiplier while boosted */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 1, ClampMax = 10))
	float DamageBoostMultiplier = 2.0f;

	/** Duration of damage boost */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 60))
	float DamageBoostDuration = 10.0f;

	/** Chance to get score boost on kill */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 1))
	float ScoreBoostChance = 0.2f;

	/** Score multiplier while boosted */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 1, ClampMax = 10))
	float ScoreBoostMultiplier = 2.0f;

	/** Duration of score boost */
	UPROPERTY(EditAnywhere, Category="Buffs", meta = (ClampMin = 0, ClampMax = 60))
	float ScoreBoostDuration = 15.0f;

	/** Controller that last damaged this character */
	AController* LastDamageInstigator = nullptr;

	/** Whether damage boost is active */
	bool bHasDamageBoost = false;

	/** Whether score boost is active */
	bool bHasScoreBoost = false;

	FTimerHandle DamageBoostTimer;
	FTimerHandle ScoreBoostTimer;

	void ActivateDamageBoost();
	void DeactivateDamageBoost();
	void ActivateScoreBoost();
	void DeactivateScoreBoost();

	/** Sends a buff notification to the owning client only */
	void ShowBuffMessage(const FString& Message, FColor Color);

public:

	/** Bullet count updated delegate */
	FBulletCountUpdatedDelegate OnBulletCountUpdated;

	/** Damaged delegate */
	FDamagedDelegate OnDamaged;

public:

	/** Constructor */
	AShooterCharacter();

protected:

	/** Property replication */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** Called when CurrentHP replicates to clients */
	UFUNCTION()
	void OnRep_CurrentHP();

	/** Called when CurrentWeapon replicates to clients */
	UFUNCTION()
	void OnRep_CurrentWeapon();

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	/** Gameplay cleanup */
	virtual void EndPlay(EEndPlayReason::Type EndPlayReason) override;

	/** Set up input action bindings */
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

	/** Handle incoming damage */
	virtual float TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

public:

	/** Handles aim inputs from either controls or UI interfaces */
	virtual void DoAim(float Yaw, float Pitch) override;

	/** Handles move inputs from either controls or UI interfaces */
	virtual void DoMove(float Right, float Forward)  override;

	/** Handles jump start inputs from either controls or UI interfaces */
	virtual void DoJumpStart()  override;

	/** Handles jump end inputs from either controls or UI interfaces */
	virtual void DoJumpEnd()  override;

	/** Handles start firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStartFiring();

	/** Handles stop firing input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoStopFiring();

	/** Called by timer to stream aim target updates to server */
	void StreamAimTarget();

	/** Server RPC for start firing */
	UFUNCTION(Server, Reliable)
	void Server_StartFiring(FVector ClientAimTarget);

	/** Server RPC for stop firing */
	UFUNCTION(Server, Reliable)
	void Server_StopFiring();

	/** Server RPC for updating aim target during full-auto fire */
	UFUNCTION(Server, Unreliable)
	void Server_UpdateAimTarget(FVector ClientAimTarget);

	/** Handles switch weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoSwitchWeapon();

	/** Server RPC for weapon switch */
	UFUNCTION(Server, Reliable)
	void Server_SwitchWeapon();

	/** Handles reload weapon input */
	UFUNCTION(BlueprintCallable, Category="Input")
	void DoReload();

	/** Server RPC for reload */
	UFUNCTION(Server, Reliable)
	void Server_Reload();

	/** Multicast RPC for equip montage */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayEquipMontage(UAnimMontage* Montage);

	/** Multicast RPC for unequip montage */
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

	/** Returns true if the character already owns a weapon of the given class */
	AShooterWeapon* FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const;

	/** Internal weapon switch logic (runs on server only) */
	void SwitchWeapon_Internal();

	/** Swaps the actual weapon and ABP, then plays equip animation */
	void PerformWeaponSwap();

	/** Called after the equip animation to finalize the switch */
	void FinishWeaponSwitch();

	/** Called when this character's HP is depleted */
	void Die();

	/** Multicast RPC to play death effects on all clients */
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnDeath();

	/** Called to allow Blueprint code to react to this character's death */
	UFUNCTION(BlueprintImplementableEvent, Category="Shooter", meta = (DisplayName = "On Death"))
	void BP_OnDeath();

	/** Called from the respawn timer to destroy this character and force the PC to respawn */
	void OnRespawn();

public:

	/** Returns true if the character is dead */
	bool IsDead() const;

	/** Returns current HP */
	float GetCurrentHP() const { return CurrentHP; }

	/** Returns max HP */
	float GetMaxHP() const { return MaxHP; }

	/** Grants kill rewards: ammo + random buffs */
	void GrantKillRewards();

	/** Returns the score multiplier for scoring */
	float GetScoreMultiplier() const { return bHasScoreBoost ? ScoreBoostMultiplier : 1.0f; }
};
