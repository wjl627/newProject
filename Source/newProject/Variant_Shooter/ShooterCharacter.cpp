// Copyright Epic Games, Inc. All Rights Reserved.


#include "ShooterCharacter.h"
#include "ShooterWeapon.h"
#include "EnhancedInputComponent.h"
#include "Components/InputComponent.h"
#include "Components/PawnNoiseEmitterComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "TimerManager.h"
#include "ShooterGameMode.h"
#include "Net/UnrealNetwork.h"
#include "ShooterPlayerController.h"

AShooterCharacter::AShooterCharacter()
{
	// create the noise emitter component
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// configure movement
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 600.0f, 0.0f);
}

void AShooterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterCharacter, CurrentHP);
	DOREPLIFETIME(AShooterCharacter, CurrentWeapon);
}

void AShooterCharacter::OnRep_CurrentHP()
{
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));
}

void AShooterCharacter::OnRep_CurrentWeapon()
{
	if (CurrentWeapon)
	{
		OnWeaponActivated(CurrentWeapon);
	}
}

void AShooterCharacter::BeginPlay()
{
	Super::BeginPlay();

	// reset HP to max
	CurrentHP = MaxHP;

	// identify this character as a player for AI perception
	Tags.Add(FName("Player"));

	// update the HUD
	OnDamaged.Broadcast(1.0f);

	// spawn default weapons (server / standalone only)
	if (HasAuthority())
	{
		for (const TSubclassOf<AShooterWeapon>& WeaponClass : DefaultWeapons)
		{
			AddWeaponClass(WeaponClass);
		}

		// switch to the first weapon in the array so array order = priority
		if (OwnedWeapons.Num() > 1)
		{
			CurrentWeapon->DeactivateWeapon();
			CurrentWeapon = OwnedWeapons[0];
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// clear timers
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(AimUpdateTimer);
	GetWorld()->GetTimerManager().ClearTimer(DamageBoostTimer);
	GetWorld()->GetTimerManager().ClearTimer(ScoreBoostTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// base class handles move, aim and jump inputs
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// Set up action bindings
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// Firing
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// Switch weapon
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AShooterCharacter::DoSwitchWeapon);

		// Reload
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AShooterCharacter::DoReload);
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// only the server applies damage
	if (!HasAuthority())
	{
		return 0.0f;
	}

	// ignore if already dead
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// remember who damaged us for kill credit
	LastDamageInstigator = EventInstigator;

	// Reduce HP
	CurrentHP -= Damage;

	// update the HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	// Have we depleted HP?
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// only route inputs if the character is not dead
	if (!IsDead())
	{
		Super::DoJumpEnd();
	}
}

void AShooterCharacter::DoStartFiring()
{
	if (CurrentWeapon && !IsDead() && !bIsSwitchingWeapon)
	{
		if (HasAuthority())
		{
			CurrentWeapon->StartFiring();
		}
		else
		{
			Server_StartFiring(GetWeaponTargetLocation());

			// Stream aim updates to server during full-auto fire
			if (CurrentWeapon->IsFullAuto())
			{
				GetWorld()->GetTimerManager().SetTimer(
					AimUpdateTimer,
					this,
					&AShooterCharacter::StreamAimTarget,
					FMath::Max(0.05f, CurrentWeapon->GetRefireRate() * 0.5f),
					true
				);
			}
		}
	}
}

void AShooterCharacter::DoStopFiring()
{
	if (!HasAuthority())
	{
		GetWorld()->GetTimerManager().ClearTimer(AimUpdateTimer);
	}

	if (CurrentWeapon && !IsDead())
	{
		if (HasAuthority())
		{
			CurrentWeapon->StopFiring();
		}
		else
		{
			Server_StopFiring();
		}
	}
}

void AShooterCharacter::Server_StartFiring_Implementation(FVector ClientAimTarget)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StartFiring(ClientAimTarget);
	}
}

void AShooterCharacter::Server_StopFiring_Implementation()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}
}

void AShooterCharacter::Server_UpdateAimTarget_Implementation(FVector ClientAimTarget)
{
	if (CurrentWeapon)
	{
		CurrentWeapon->UpdateOverrideAimTarget(ClientAimTarget);
	}
}

void AShooterCharacter::StreamAimTarget()
{
	if (CurrentWeapon)
	{
		Server_UpdateAimTarget(GetWeaponTargetLocation());
	}
}

void AShooterCharacter::DoReload()
{
	if (CurrentWeapon && !IsDead() && !bIsSwitchingWeapon && !CurrentWeapon->IsReloading())
	{
		if (HasAuthority())
		{
			CurrentWeapon->Reload();
		}
		else
		{
			Server_Reload();
		}
	}
}

void AShooterCharacter::Server_Reload_Implementation()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Reload();
	}
}

void AShooterCharacter::DoSwitchWeapon()
{
	if (!HasAuthority())
	{
		Server_SwitchWeapon();
		return;
	}

	SwitchWeapon_Internal();
}

void AShooterCharacter::Server_SwitchWeapon_Implementation()
{
	SwitchWeapon_Internal();
}

void AShooterCharacter::SwitchWeapon_Internal()
{
	if (OwnedWeapons.Num() < 2 || IsDead() || bIsSwitchingWeapon)
	{
		return;
	}

	bIsSwitchingWeapon = true;

	// stop firing
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}

	// find the next weapon index
	const int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);
	PendingWeaponIndex = (WeaponIndex == OwnedWeapons.Num() - 1) ? 0 : WeaponIndex + 1;

	// if the current weapon has an unequip montage, play it before swapping
	if (CurrentWeapon && CurrentWeapon->GetUnequipMontage())
	{
		Multicast_PlayUnequipMontage(CurrentWeapon->GetUnequipMontage());

		const float UnequipDuration = CurrentWeapon->GetUnequipMontage()->GetPlayLength();

		GetWorld()->GetTimerManager().SetTimer(
			WeaponSwitchTimerHandle,
			this,
			&AShooterCharacter::PerformWeaponSwap,
			UnequipDuration,
			false
		);
	}
	else
	{
		// no unequip animation — swap immediately
		PerformWeaponSwap();
	}
}

void AShooterCharacter::PerformWeaponSwap()
{
	if (IsDead())
	{
		GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchTimerHandle);
		bIsSwitchingWeapon = false;
		return;
	}

	// deactivate old weapon (hides mesh, but ABP is still the old one)
	if (CurrentWeapon)
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// activate new weapon (swaps ABP, but hide mesh until equip anim plays)
	CurrentWeapon = OwnedWeapons[PendingWeaponIndex];
	CurrentWeapon->ActivateWeapon();

	// if the new weapon has an equip montage, hide it for one frame
	// so the ABP can initialize, then unhide and play the montage together
	if (CurrentWeapon && CurrentWeapon->GetEquipMontage())
	{
		CurrentWeapon->SetActorHiddenInGame(true);

		FTimerHandle EquipTimer;
		GetWorld()->GetTimerManager().SetTimer(
			EquipTimer,
			[this]()
			{
				if (CurrentWeapon)
				{
					CurrentWeapon->SetActorHiddenInGame(false);
					Multicast_PlayEquipMontage(CurrentWeapon->GetEquipMontage());

					const float EquipDuration = CurrentWeapon->GetEquipMontage()->GetPlayLength();

					GetWorld()->GetTimerManager().SetTimer(
						WeaponSwitchTimerHandle,
						this,
						&AShooterCharacter::FinishWeaponSwitch,
						EquipDuration,
						false
					);
				}
			},
			0.02f,
			false
		);
	}
	else
	{
		// no equip animation — finish immediately
		FinishWeaponSwitch();
	}
}

void AShooterCharacter::FinishWeaponSwitch()
{
	bIsSwitchingWeapon = false;
}

void AShooterCharacter::Multicast_PlayEquipMontage_Implementation(UAnimMontage* Montage)
{
	PlayEquipMontage(Montage);
}

void AShooterCharacter::Multicast_PlayUnequipMontage_Implementation(UAnimMontage* Montage)
{
	PlayUnequipMontage(Montage);
}

void AShooterCharacter::AttachWeaponMeshes(AShooterWeapon* Weapon)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// attach the weapon actor
	Weapon->AttachToActor(this, AttachmentRule);

	// attach the weapon meshes
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);

}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	if (Montage && GetFirstPersonMesh())
	{
		GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Montage);
	}
}

void AShooterCharacter::PlayEquipMontage(UAnimMontage* Montage)
{
	if (Montage && GetFirstPersonMesh())
	{
		GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Montage);
	}
}

void AShooterCharacter::PlayUnequipMontage(UAnimMontage* Montage)
{
	if (Montage && GetFirstPersonMesh())
	{
		GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Montage);
	}
}

void AShooterCharacter::PlayReloadMontage(UAnimMontage* Montage)
{
	if (Montage && GetFirstPersonMesh())
	{
		GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Montage);
	}
}

void AShooterCharacter::AddWeaponRecoil(float Recoil)
{
	// apply the recoil as pitch input
	AddControllerPitchInput(Recoil);
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// trace ahead from the camera viewpoint
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// return either the impact point or the trace end
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// do we already own this weapon?
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// spawn the new weapon
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// add the weapon to the owned list
			OwnedWeapons.Add(AddedWeapon);

			// if we have an existing weapon, deactivate it
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// switch to the new weapon
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// update the bullet counter
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// set the character mesh AnimInstances
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// unused
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// unused
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// check each owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// weapon not found
	return nullptr;

}

void AShooterCharacter::Die()
{
	// only the server kills the character
	if (!HasAuthority())
	{
		return;
	}

	// deactivate the weapon and clear buffs
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
		CurrentWeapon->SetDamageMultiplier(1.0f);
	}

	// clear active buff timers
	GetWorld()->GetTimerManager().ClearTimer(DamageBoostTimer);
	GetWorld()->GetTimerManager().ClearTimer(ScoreBoostTimer);
	bHasDamageBoost = false;
	bHasScoreBoost = false;

	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		// grant kill score + rewards to the killer (AddKillScore handles both)
		GM->AddKillScore(LastDamageInstigator);

		// halve the dead player's score
		GM->OnPlayerDeath(GetController());
	}

	// grant the death tag to the character
	Tags.Add(DeathTag);

	// stop character movement
	GetCharacterMovement()->StopMovementImmediately();

	// disable controls
	DisableInput(nullptr);

	// reset the bullet counter UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// call the BP handler
	Multicast_OnDeath();

	// schedule character respawn
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::Multicast_OnDeath_Implementation()
{
	BP_OnDeath();
}

void AShooterCharacter::OnRespawn()
{
	// destroy the character to force the PC to respawn
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// the character is dead if their current HP drops to zero
	return CurrentHP <= 0.0f;
}

// ---- Buff System ----

void AShooterCharacter::GrantKillRewards()
{
	if (!HasAuthority())
	{
		return;
	}

	// Add one reserve magazine to every owned weapon
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon)
		{
			Weapon->AddReserveMagazines(1);
		}
	}

	ShowBuffMessage(TEXT("Kill Reward: +1 Magazine"), FColor::Green);

	// Roll for health restore
	if (FMath::FRand() < HealthRestoreChance)
	{
		const float PrevHP = CurrentHP;
		CurrentHP = FMath::Min(CurrentHP + HealthRestoreAmount, MaxHP);
		if (CurrentHP > PrevHP)
		{
			OnDamaged.Broadcast(CurrentHP / MaxHP);
			ShowBuffMessage(FString::Printf(TEXT("Kill Reward: +%.0f HP"), CurrentHP - PrevHP), FColor::Green);
		}
	}

	// Roll for damage boost
	if (!bHasDamageBoost && FMath::FRand() < DamageBoostChance)
	{
		ActivateDamageBoost();
	}

	// Roll for score boost
	if (!bHasScoreBoost && FMath::FRand() < ScoreBoostChance)
	{
		ActivateScoreBoost();
	}
}

void AShooterCharacter::ActivateDamageBoost()
{
	bHasDamageBoost = true;
	if (CurrentWeapon)
	{
		CurrentWeapon->SetDamageMultiplier(DamageBoostMultiplier);
	}
	ShowBuffMessage(FString::Printf(TEXT("Damage Boost x%.0f! (%.0fs)"), DamageBoostMultiplier, DamageBoostDuration), FColor::Orange);
	GetWorld()->GetTimerManager().SetTimer(DamageBoostTimer, this, &AShooterCharacter::DeactivateDamageBoost, DamageBoostDuration, false);
}

void AShooterCharacter::DeactivateDamageBoost()
{
	bHasDamageBoost = false;
	if (CurrentWeapon)
	{
		CurrentWeapon->SetDamageMultiplier(1.0f);
	}
	ShowBuffMessage(TEXT("Damage Boost Ended"), FColor::Red);
}

void AShooterCharacter::ActivateScoreBoost()
{
	bHasScoreBoost = true;
	ShowBuffMessage(FString::Printf(TEXT("Score Boost x%.0f! (%.0fs)"), ScoreBoostMultiplier, ScoreBoostDuration), FColor::Cyan);
	GetWorld()->GetTimerManager().SetTimer(ScoreBoostTimer, this, &AShooterCharacter::DeactivateScoreBoost, ScoreBoostDuration, false);
}

void AShooterCharacter::DeactivateScoreBoost()
{
	bHasScoreBoost = false;
	ShowBuffMessage(TEXT("Score Boost Ended"), FColor::Red);
}

void AShooterCharacter::ShowBuffMessage(const FString& Message, FColor Color)
{
	if (AShooterPlayerController* PC = Cast<AShooterPlayerController>(GetController()))
	{
		PC->Client_ShowBuffMessage(Message, Color);
	}
}
