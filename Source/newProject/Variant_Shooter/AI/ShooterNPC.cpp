// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/AI/ShooterNPC.h"
#include "ShooterWeapon.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "ShooterGameMode.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"

void AShooterNPC::BeginPlay()
{
	Super::BeginPlay();

	// 生成武器（仅服务器）
	if (!HasAuthority())
	{
		return;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	Weapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

	// NPC 拥有无限备用弹药
	if (Weapon)
	{
		Weapon->AddReserveMagazines(Weapon->GetMaxReserveMagazines());
	}
}

void AShooterNPC::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清除死亡计时器
	GetWorld()->GetTimerManager().ClearTimer(DeathTimer);
}

void AShooterNPC::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AShooterNPC, CurrentHP);
	DOREPLIFETIME(AShooterNPC, bIsDead);
	DOREPLIFETIME(AShooterNPC, Weapon);
}

void AShooterNPC::OnRep_bIsDead()
{
	if (bIsDead)
	{
		// 在第三人称网格上启用布娃娃物理
		GetMesh()->SetCollisionProfileName(RagdollCollisionProfile);
		GetMesh()->SetSimulatePhysics(true);
		GetMesh()->SetPhysicsBlendWeight(1.0f);

		// 停止移动
		GetCharacterMovement()->StopMovementImmediately();
	}
}

float AShooterNPC::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 只有服务器应用伤害
	if (!HasAuthority())
	{
		return 0.0f;
	}

	// 已经死亡则忽略
	if (bIsDead)
	{
		return 0.0f;
	}

	// 记住伤害来源以便记录击杀
	LastDamageInstigator = EventInstigator;

	// 减少 HP
	CurrentHP -= Damage;

	// HP 是否已耗尽？
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	return Damage;
}

void AShooterNPC::AttachWeaponMeshes(AShooterWeapon* WeaponToAttach)
{
	const FAttachmentTransformRules AttachmentRule(EAttachmentRule::SnapToTarget, false);

	// 挂接武器 Actor
	WeaponToAttach->AttachToActor(this, AttachmentRule);

	// 挂接武器网格
	WeaponToAttach->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	WeaponToAttach->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, ThirdPersonWeaponSocket);
}

void AShooterNPC::PlayFiringMontage(UAnimMontage* Montage)
{
	// 未使用
}

void AShooterNPC::AddWeaponRecoil(float Recoil)
{
	// 未使用
}

void AShooterNPC::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	// 未使用
}

FVector AShooterNPC::GetWeaponTargetLocation()
{
	// 从相机位置开始瞄准
	const FVector AimSource = GetFirstPersonCameraComponent()->GetComponentLocation();

	FVector AimDir, AimTarget = FVector::ZeroVector;

	// 是否有瞄准目标？
	if (IsValid(CurrentAimTarget) && !CurrentAimTarget->ActorHasTag(DeathTag))
	{
		// 瞄准 Actor 的位置
		AimTarget = CurrentAimTarget->GetActorLocation();

		// 应用垂直偏移以瞄准头部/脚部
		AimTarget.Z += FMath::RandRange(MinAimOffsetZ, MaxAimOffsetZ);

		// 获取瞄准方向并在锥形范围内施加随机性
		AimDir = (AimTarget - AimSource).GetSafeNormal();
		AimDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(AimDir, AimVarianceHalfAngle);

		
	} else {

		// 没有瞄准目标，直接使用相机朝向
		AimDir = UKismetMathLibrary::RandomUnitVectorInConeInDegrees(GetFirstPersonCameraComponent()->GetForwardVector(), AimVarianceHalfAngle);

	}

	// 计算无遮挡时的瞄准目标位置
	AimTarget = AimSource + (AimDir * AimRange);

	// 执行可见性检测以查看是否有遮挡
	FHitResult OutHit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, AimSource, AimTarget, ECC_Visibility, QueryParams);

	// 返回命中点或检测终点
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterNPC::AddWeaponClass(const TSubclassOf<AShooterWeapon>& InWeaponClass)
{
	// 未使用
}

void AShooterNPC::OnWeaponActivated(AShooterWeapon* InWeapon)
{
	// 未使用
}

void AShooterNPC::OnWeaponDeactivated(AShooterWeapon* InWeapon)
{
	// 未使用
}

void AShooterNPC::OnSemiWeaponRefire()
{
	// 我们仍在射击吗？
	if (bIsShooting)
	{
		// 开火
		Weapon->StartFiring();
	}
}

void AShooterNPC::PlayEquipMontage(UAnimMontage* Montage)
{
	// 未使用（NPC 不播放装备动画）
}

void AShooterNPC::PlayUnequipMontage(UAnimMontage* Montage)
{
	// 未使用（NPC 不播放卸下武器动画）
}

void AShooterNPC::PlayReloadMontage(UAnimMontage* Montage)
{
	// 未使用（NPC 不播放换弹动画）
}

void AShooterNPC::Die()
{
	// 只有服务器能击杀此角色
	if (!HasAuthority())
	{
		return;
	}

	// 已经死亡则忽略
	if (bIsDead)
	{
		return;
	}

	// 设置死亡标记
	bIsDead = true;

	// 为角色添加死亡标签
	Tags.Add(DeathTag);

	// 调用委托
	OnPawnDeath.Broadcast();

	// 向击杀我们的玩家发放击杀分数
	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		GM->AddKillScore(LastDamageInstigator);
	}

	// 禁用胶囊体碰撞
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 停止移动
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->StopActiveMovement();

	// 在第三人称网格上启用布娃娃物理
	GetMesh()->SetCollisionProfileName(RagdollCollisionProfile);
	GetMesh()->SetSimulatePhysics(true);
	GetMesh()->SetPhysicsBlendWeight(1.0f);

	// 安排 Actor 销毁
	GetWorld()->GetTimerManager().SetTimer(DeathTimer, this, &AShooterNPC::DeferredDestruction, DeferredDestructionTime, false);
}

void AShooterNPC::DeferredDestruction()
{
	Destroy();
}

void AShooterNPC::StartShooting(AActor* ActorToShoot)
{
	// 保存瞄准目标
	CurrentAimTarget = ActorToShoot;

	// 设置标记
	bIsShooting = true;

	// 通知武器
	Weapon->StartFiring();
}

void AShooterNPC::StopShooting()
{
	bIsShooting = false;
	CurrentAimTarget = nullptr;

	Weapon->StopFiring();
}
