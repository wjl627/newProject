// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterWeapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterBulletTracer.h"
#include "ShooterWeaponHolder.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
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

	// 记录静止变换，以便后坐力冲击能回到该位置
	FirstPersonMeshRestTransform = FirstPersonMesh->GetRelativeTransform();
	ThirdPersonMeshRestTransform = ThirdPersonMesh->GetRelativeTransform();
	bRestTransformsCaptured = true;
}

void AShooterWeapon::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (WeaponKickAlpha > 0.0f)
	{
		WeaponKickAlpha = FMath::FInterpTo(WeaponKickAlpha, 0.0f, DeltaSeconds, 1.0f / FMath::Max(WeaponKickDuration, KINDA_SMALL_NUMBER));
		ApplyWeaponKick();
	}
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

	// 根据动画或回退时间确定换弹时长
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

FVector AShooterWeapon::GetMuzzleLocation() const
{
	auto TryGetSocketLocation = [this](USkeletalMeshComponent* Mesh, FVector& OutLocation) -> bool
	{
		if (Mesh && !MuzzleSocketName.IsNone() && Mesh->DoesSocketExist(MuzzleSocketName))
		{
			OutLocation = Mesh->GetSocketLocation(MuzzleSocketName);
			return true;
		}
		return false;
	};

	// 选择本地观看者可见的网格
	USkeletalMeshComponent* VisualMesh = ThirdPersonMesh;
	const AController* OwnerController = PawnOwner ? PawnOwner->GetController() : nullptr;
	if (OwnerController && OwnerController->IsLocalPlayerController())
	{
		VisualMesh = FirstPersonMesh;
	}

	FVector SocketLocation = FVector::ZeroVector;
	if (TryGetSocketLocation(VisualMesh, SocketLocation))
	{
		return SocketLocation;
	}

	// 回退：尝试另一个网格
	if (VisualMesh != FirstPersonMesh && TryGetSocketLocation(FirstPersonMesh, SocketLocation))
	{
		return SocketLocation;
	}
	if (VisualMesh != ThirdPersonMesh && TryGetSocketLocation(ThirdPersonMesh, SocketLocation))
	{
		return SocketLocation;
	}

	return VisualMesh ? VisualMesh->GetComponentLocation() : GetActorLocation();
}

void AShooterWeapon::TriggerWeaponKick()
{
	if (WeaponKickBack <= 0.0f && WeaponKickUp <= 0.0f)
	{
		return;
	}

	WeaponKickAlpha = 1.0f;
	ApplyWeaponKick();
}

void AShooterWeapon::ApplyWeaponKick()
{
	if (!bRestTransformsCaptured)
	{
		return;
	}

	const auto ApplyKickToMesh = [this](USkeletalMeshComponent* Mesh, const FTransform& RestTransform)
	{
		if (!Mesh)
		{
			return;
		}

		// 第一人称视角下，相对持有者相机施加冲击，使枪在屏幕上
		// 始终向后上方移动。第三人称视角则使用持有者自身的轴向。
		FVector BackDir = -GetActorForwardVector();
		FVector UpDir = FVector::UpVector;
		if (Mesh == FirstPersonMesh && PawnOwner)
		{
			if (const UCameraComponent* Camera = PawnOwner->FindComponentByClass<UCameraComponent>())
			{
				BackDir = -Camera->GetForwardVector();
				UpDir = Camera->GetUpVector();
			}
		}

		const FVector WorldOffset = (BackDir * WeaponKickBack + UpDir * WeaponKickUp) * WeaponKickAlpha;

		// 将世界空间的冲击转换到网格的父空间，使无论武器
		// 如何挂接，效果看起来都一致
		FVector LocalOffset = FVector::ZeroVector;
		if (USceneComponent* Parent = Mesh->GetAttachParent())
		{
			LocalOffset = Parent->GetComponentTransform().InverseTransformVector(WorldOffset);
		}

		Mesh->SetRelativeLocation(RestTransform.GetLocation() + LocalOffset);
	};

	ApplyKickToMesh(FirstPersonMesh, FirstPersonMeshRestTransform);
	ApplyKickToMesh(ThirdPersonMesh, ThirdPersonMeshRestTransform);
}

void AShooterWeapon::FireProjectile(const FVector& TargetLocation)
{
	// 从枪口开始射击，使子弹从枪口而不是眼睛位置射出。
	// 本地控制的第一人称玩家保留基于相机的射线，以保证
	// 准星准确。AI Pawn（NPC）始终从枪口开火。
	const AController* OwnerController = PawnOwner ? PawnOwner->GetController() : nullptr;
	const FVector TraceOrigin = (OwnerController && OwnerController->IsLocalPlayerController())
		? PawnOwner->GetPawnViewLocation()
		: GetMuzzleLocation();
	const FVector AimDir = (TargetLocation - TraceOrigin).GetSafeNormal();
	const FVector TraceStart = TraceOrigin + AimDir * MuzzleOffset;
	const FVector TraceEnd = TraceStart + AimDir * FMath::Max(MaxFireRange, FVector::Dist(TraceOrigin, TargetLocation) + 500.0f);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.AddIgnoredActor(this);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);
	ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FHitResult HitResult;
	const bool bHit = GetWorld()->LineTraceSingleByObjectType(HitResult, TraceStart, TraceEnd, ObjectParams, QueryParams);
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

	--CurrentBullets;

	Multicast_OnFireEffects(CurrentBullets, ImpactPoint);

	if (CurrentBullets <= 0)
	{
		Reload();
	}
}

void AShooterWeapon::Multicast_OnFireEffects_Implementation(int32 BulletCount, const FVector& ImpactPoint)
{
	CurrentBullets = BulletCount;

	if (WeaponOwner)
	{
		WeaponOwner->PlayFiringMontage(FiringMontage);
		WeaponOwner->AddWeaponRecoil(FiringRecoil);
		WeaponOwner->UpdateWeaponHUD(BulletCount, MagazineSize);
	}

	TriggerWeaponKick();

	if (TracerClass)
	{
		const FVector MuzzleLocation = GetMuzzleLocation();

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		if (AShooterBulletTracer* Tracer = GetWorld()->SpawnActor<AShooterBulletTracer>(TracerClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams))
		{
			const FVector Dir = (ImpactPoint - MuzzleLocation).GetSafeNormal();
			if (!Dir.IsNearlyZero())
			{
				const FVector TracerStart = MuzzleLocation + Dir * MuzzleOffset;
				Tracer->Initialize(TracerStart, ImpactPoint);
			}
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
