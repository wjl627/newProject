// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterBulletTracer.h"
#include "ShooterProjectile.h"
#include "ShooterWeaponHolder.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"

AShooterWeapon::AShooterWeapon()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = true;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
	FirstPersonMesh->SetupAttachment(RootComponent);
	FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	FirstPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	FirstPersonMesh->bOnlyOwnerSee = true;

	ThirdPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Third Person Mesh"));
	ThirdPersonMesh->SetupAttachment(RootComponent);
	ThirdPersonMesh->SetCollisionProfileName(FName("NoCollision"));
	ThirdPersonMesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::WorldSpaceRepresentation);
	ThirdPersonMesh->bOwnerNoSee = true;
}

void AShooterWeapon::BeginPlay()
{
	Super::BeginPlay();

	GetOwner()->OnDestroyed.AddDynamic(this, &AShooterWeapon::OnOwnerDestroyed);

	WeaponOwner = Cast<IShooterWeaponHolder>(GetOwner());
	PawnOwner = Cast<APawn>(GetOwner());

	CurrentBullets = MagazineSize;

	WeaponOwner->AttachWeaponMeshes(this);
}

void AShooterWeapon::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
	GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
}

void AShooterWeapon::OnOwnerDestroyed(AActor* DestroyedActor)
{
	Destroy();
}

void AShooterWeapon::ActivateWeapon()
{
	SetActorHiddenInGame(false);
	WeaponOwner->OnWeaponActivated(this);
}

void AShooterWeapon::DeactivateWeapon()
{
	StopFiring();

	if (bIsReloading)
	{
		bIsReloading = false;
		GetWorld()->GetTimerManager().ClearTimer(ReloadTimer);
	}

	SetActorHiddenInGame(true);
	WeaponOwner->OnWeaponDeactivated(this);
}

void AShooterWeapon::StartFiring(const FVector& OverrideTarget)
{
	if (bIsReloading || CurrentBullets <= 0)
	{
		return;
	}

	bIsFiring = true;

	if (!OverrideTarget.IsZero())
	{
		OverrideAimTarget = OverrideTarget;
	}

	const float TimeSinceLastShot = GetWorld()->GetTimeSeconds() - TimeOfLastShot;

	if (TimeSinceLastShot > RefireRate)
	{
		Fire();
	}
	else
	{
		if (bFullAuto)
		{
			GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, TimeSinceLastShot, false);
		}
	}
}

void AShooterWeapon::StopFiring()
{
	bIsFiring = false;
	OverrideAimTarget = FVector::ZeroVector;
	GetWorld()->GetTimerManager().ClearTimer(RefireTimer);
}

void AShooterWeapon::Reload()
{
	if (!CanReload())
	{
		return;
	}

	bIsReloading = true;

	// Determine reload duration from montage or fallback time
	float Duration = ReloadTime;
	if (ReloadMontage)
	{
		Duration = ReloadMontage->GetPlayLength();
	}
	else if (WeaponOwner && WeaponOwner->GetWeaponTargetLocation().IsZero())
	{
		Duration = ReloadTime;
	}

	Multicast_OnReloadEffects();
	GetWorld()->GetTimerManager().SetTimer(ReloadTimer, this, &AShooterWeapon::FinishReload, Duration, false);
}

void AShooterWeapon::FinishReload()
{
	bIsReloading = false;
	--ReserveMagazines;
	CurrentBullets = MagazineSize;

	if (WeaponOwner)
	{
		WeaponOwner->UpdateWeaponHUD(CurrentBullets, MagazineSize);
	}

	if (bIsFiring)
	{
		StartFiring();
	}
}

void AShooterWeapon::AddReserveMagazines(int32 Count)
{
	ReserveMagazines = FMath::Min(ReserveMagazines + Count, MaxReserveMagazines);
}

void AShooterWeapon::Fire()
{
	if (!bIsFiring || bIsReloading || CurrentBullets <= 0)
	{
		return;
	}

	const FVector AimTarget = OverrideAimTarget.IsZero() ? WeaponOwner->GetWeaponTargetLocation() : OverrideAimTarget;
	OverrideAimTarget = FVector::ZeroVector;
	FireProjectile(AimTarget);

	TimeOfLastShot = GetWorld()->GetTimeSeconds();

	MakeNoise(ShotLoudness, PawnOwner, PawnOwner->GetActorLocation(), ShotNoiseRange, ShotNoiseTag);

	if (bFullAuto)
	{
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::Fire, RefireRate, false);
	}
	else
	{
		GetWorld()->GetTimerManager().SetTimer(RefireTimer, this, &AShooterWeapon::FireCooldownExpired, RefireRate, false);
	}
}

void AShooterWeapon::FireCooldownExpired()
{
	WeaponOwner->OnSemiWeaponRefire();
}

void AShooterWeapon::FireProjectile(const FVector& TargetLocation)
{
	const FVector EyeLoc = PawnOwner ? PawnOwner->GetPawnViewLocation() : FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	const FVector AimDir = (TargetLocation - EyeLoc).GetSafeNormal();
	const FVector TraceEnd = EyeLoc + AimDir * FMath::Max(MaxFireRange, FVector::Dist(EyeLoc, TargetLocation) + 500.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, EyeLoc, TraceEnd, ObjectParams, QueryParams);
	const FVector ImpactPoint = bHit ? HitResult.ImpactPoint : HitResult.TraceEnd;

	if (bHit)
	{
		UGameplayStatics::ApplyDamage(HitResult.GetActor(), HitDamage * DamageMultiplier, PawnOwner->GetController(), this, UDamageType::StaticClass());

		if (UPrimitiveComponent* HitComp = HitResult.GetComponent())
		{
			if (HitComp->IsSimulatingPhysics())
			{
				HitComp->AddImpulseAtLocation(-AimDir * 100.0f, HitResult.ImpactPoint);
			}
		}
	}

	const FVector MuzzleLoc = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);

	--CurrentBullets;

	Multicast_OnFireEffects(CurrentBullets, MuzzleLoc, ImpactPoint);

	if (CurrentBullets <= 0)
	{
		Reload();
	}
}

void AShooterWeapon::Multicast_OnFireEffects_Implementation(int32 BulletCount, const FVector& MuzzleLocation, const FVector& ImpactPoint)
{
	CurrentBullets = BulletCount;

	if (WeaponOwner)
	{
		WeaponOwner->PlayFiringMontage(FiringMontage);
		WeaponOwner->AddWeaponRecoil(FiringRecoil);
		WeaponOwner->UpdateWeaponHUD(BulletCount, MagazineSize);
	}

	if (TracerClass)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AShooterBulletTracer* Tracer = GetWorld()->SpawnActor<AShooterBulletTracer>(TracerClass, MuzzleLocation, FRotator::ZeroRotator, SpawnParams))
		{
			Tracer->Initialize(MuzzleLocation, ImpactPoint);
		}
	}
}

void AShooterWeapon::Multicast_OnReloadEffects_Implementation()
{
	if (WeaponOwner && ReloadMontage)
	{
		WeaponOwner->PlayReloadMontage(ReloadMontage);
	}
}

FTransform AShooterWeapon::CalculateProjectileSpawnTransform(const FVector& TargetLocation) const
{
	const FVector MuzzleLoc = FirstPersonMesh->GetSocketLocation(MuzzleSocketName);
	const FVector SpawnLoc = MuzzleLoc + ((TargetLocation - MuzzleLoc).GetSafeNormal() * MuzzleOffset);
	const FRotator AimRot = UKismetMathLibrary::FindLookAtRotation(SpawnLoc, TargetLocation + (UKismetMathLibrary::RandomUnitVector() * AimVariance));

	return FTransform(AimRot, SpawnLoc, FVector::OneVector);
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetFirstPersonAnimInstanceClass() const
{
	return FirstPersonAnimInstanceClass;
}

const TSubclassOf<UAnimInstance>& AShooterWeapon::GetThirdPersonAnimInstanceClass() const
{
	return ThirdPersonAnimInstanceClass;
}

void AShooterWeapon::PlayEquipMontage()
{
	if (EquipMontage && WeaponOwner)
	{
		WeaponOwner->PlayEquipMontage(EquipMontage);
	}
}

void AShooterWeapon::PlayUnequipMontage()
{
	if (UnequipMontage && WeaponOwner)
	{
		WeaponOwner->PlayUnequipMontage(UnequipMontage);
	}
}
