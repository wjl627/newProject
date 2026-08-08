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
	// 创建噪音发射器组件
	PawnNoiseEmitter = CreateDefaultSubobject<UPawnNoiseEmitterComponent>(TEXT("Pawn Noise Emitter"));

	// 配置移动
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

	// 将 HP 重置为最大值
	CurrentHP = MaxHP;

	// 将该角色标记为玩家，供 AI 感知识别
	Tags.Add(FName("Player"));

	// 更新 HUD
	OnDamaged.Broadcast(1.0f);

	// 生成默认武器（仅服务器 / 单机）
	if (HasAuthority())
	{
		for (const TSubclassOf<AShooterWeapon>& WeaponClass : DefaultWeapons)
		{
			AddWeaponClass(WeaponClass);
		}

		// 切换到数组中的第一把武器，因此数组顺序即优先级
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

	// 清除计时器
	GetWorld()->GetTimerManager().ClearTimer(RespawnTimer);
	GetWorld()->GetTimerManager().ClearTimer(WeaponSwitchTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(AimUpdateTimer);
	GetWorld()->GetTimerManager().ClearTimer(RecoilRecoveryTimer);
	GetWorld()->GetTimerManager().ClearTimer(RecoilSettleTimer);
	GetWorld()->GetTimerManager().ClearTimer(DamageBoostTimer);
	GetWorld()->GetTimerManager().ClearTimer(ScoreBoostTimer);
}

void AShooterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	// 基类处理移动、瞄准和跳跃输入
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	// 设置操作绑定
	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		// 开火
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &AShooterCharacter::DoStartFiring);
		EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &AShooterCharacter::DoStopFiring);

		// 切换武器
		EnhancedInputComponent->BindAction(SwitchWeaponAction, ETriggerEvent::Started, this, &AShooterCharacter::DoSwitchWeapon);

		// 换弹
		EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &AShooterCharacter::DoReload);
	}

}

float AShooterCharacter::TakeDamage(float Damage, struct FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	// 只有服务器应用伤害
	if (!HasAuthority())
	{
		return 0.0f;
	}

	// 已经死亡则忽略
	if (CurrentHP <= 0.0f)
	{
		return 0.0f;
	}

	// 记住伤害来源以便记录击杀
	LastDamageInstigator = EventInstigator;

	// 减少 HP
	CurrentHP -= Damage;

	// 更新 HUD
	OnDamaged.Broadcast(FMath::Max(0.0f, CurrentHP / MaxHP));

	// HP 是否已耗尽？
	if (CurrentHP <= 0.0f)
	{
		Die();
	}

	return Damage;
}

void AShooterCharacter::DoAim(float Yaw, float Pitch)
{
	// 仅在角色未死亡时传递输入
	if (!IsDead())
	{
		Super::DoAim(Yaw, Pitch);
	}
}

void AShooterCharacter::DoMove(float Right, float Forward)
{
	// 仅在角色未死亡时传递输入
	if (!IsDead())
	{
		Super::DoMove(Right, Forward);
	}
}

void AShooterCharacter::DoJumpStart()
{
	// 仅在角色未死亡时传递输入
	if (!IsDead())
	{
		Super::DoJumpStart();
	}
}

void AShooterCharacter::DoJumpEnd()
{
	// 仅在角色未死亡时传递输入
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

			// 全自动射击期间持续向服务器发送瞄准更新
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

	// 停止开火
	if (CurrentWeapon)
	{
		CurrentWeapon->StopFiring();
	}

	// 查找下一把武器的索引
	const int32 WeaponIndex = OwnedWeapons.Find(CurrentWeapon);
	PendingWeaponIndex = (WeaponIndex == OwnedWeapons.Num() - 1) ? 0 : WeaponIndex + 1;

	// 如果当前武器有卸下动画，则在切换前播放
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
		// 没有卸下动画——立即切换
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

	// 停用旧武器（隐藏网格，但 ABP 仍是旧的）
	if (CurrentWeapon)
	{
		CurrentWeapon->DeactivateWeapon();
	}

	// 激活新武器（切换 ABP，但在装备动画播放前隐藏网格）
	CurrentWeapon = OwnedWeapons[PendingWeaponIndex];
	CurrentWeapon->ActivateWeapon();

	// 如果新武器有装备动画，先隐藏一帧
	// 让 ABP 完成初始化，再取消隐藏并同时播放动画
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
		// 没有装备动画——立即完成
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

	// 挂接武器 Actor
	Weapon->AttachToActor(this, AttachmentRule);

	// 挂接武器网格
	Weapon->GetFirstPersonMesh()->AttachToComponent(GetFirstPersonMesh(), AttachmentRule, FirstPersonWeaponSocket);
	Weapon->GetThirdPersonMesh()->AttachToComponent(GetMesh(), AttachmentRule, FirstPersonWeaponSocket);

}

void AShooterCharacter::PlayFiringMontage(UAnimMontage* Montage)
{
	if (!bPlayFiringMontage || !Montage || !GetFirstPersonMesh())
	{
		return;
	}

	GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Montage);
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
	if (IsDead())
	{
		return;
	}

	// 如果武器没有配置后坐力，则回退到角色默认值
	const float KickAmount = Recoil > 0.0f ? Recoil : DefaultWeaponRecoil;
	if (KickAmount <= 0.0f)
	{
		return;
	}

	// 给相机施加带随机性的向上冲击，并附加较小的随机水平冲击
	const float PitchKick = FMath::Min(FMath::FRandRange(KickAmount * 0.8f, KickAmount * 1.2f), MaxRecoilPitchPerShot);
	const float YawKick = FMath::FRandRange(-1.0f, 1.0f) * PitchKick * RecoilYawRatio;

	// 直接将冲击应用到控制旋转上，使方向始终垂直向上，不受输入缩放或反转设置影响。
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		FRotator ControlRotation = PC->GetControlRotation();
		ControlRotation.Pitch += PitchKick;
		ControlRotation.Yaw += YawKick;
		PC->SetControlRotation(ControlRotation);
	}

	CurrentRecoilPitch = FMath::Min(CurrentRecoilPitch + PitchKick, MaxRecoilPitch);
	CurrentRecoilYaw = FMath::Clamp(CurrentRecoilYaw + YawKick, -MaxRecoilYaw, MaxRecoilYaw);

	// 开火期间不恢复：重置稳定计时器，使相机仅在停止射击后才开始回位
	GetWorldTimerManager().ClearTimer(RecoilSettleTimer);
	GetWorldTimerManager().SetTimer(RecoilSettleTimer, this, &AShooterCharacter::StartRecoilRecovery, RecoilSettleDelay, false);
}

void AShooterCharacter::StartRecoilRecovery()
{
	if (FMath::IsNearlyZero(CurrentRecoilPitch) && FMath::IsNearlyZero(CurrentRecoilYaw))
	{
		return;
	}

	if (!GetWorldTimerManager().IsTimerActive(RecoilRecoveryTimer))
	{
		GetWorldTimerManager().SetTimer(RecoilRecoveryTimer, this, &AShooterCharacter::UpdateRecoilRecovery, 1.0f / 60.0f, true);
	}
}

void AShooterCharacter::UpdateRecoilRecovery()
{
	const float DeltaTime = 1.0f / 60.0f;
	const float Step = RecoilRecoverySpeed * DeltaTime;

	// 俯仰：向零回位
	if (FMath::Abs(CurrentRecoilPitch) > Step)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FRotator ControlRotation = PC->GetControlRotation();
			ControlRotation.Pitch -= Step;
			PC->SetControlRotation(ControlRotation);
		}
		CurrentRecoilPitch -= Step;
	}
	else
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FRotator ControlRotation = PC->GetControlRotation();
			ControlRotation.Pitch -= CurrentRecoilPitch;
			PC->SetControlRotation(ControlRotation);
		}
		CurrentRecoilPitch = 0.0f;
	}

	// 偏航：向零回位
	if (FMath::Abs(CurrentRecoilYaw) > Step)
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FRotator ControlRotation = PC->GetControlRotation();
			ControlRotation.Yaw -= FMath::Sign(CurrentRecoilYaw) * Step;
			PC->SetControlRotation(ControlRotation);
		}
		CurrentRecoilYaw -= FMath::Sign(CurrentRecoilYaw) * Step;
	}
	else
	{
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			FRotator ControlRotation = PC->GetControlRotation();
			ControlRotation.Yaw -= CurrentRecoilYaw;
			PC->SetControlRotation(ControlRotation);
		}
		CurrentRecoilYaw = 0.0f;
	}

	// 相机完全回到射击前瞄准位置后停止计时器
	if (FMath::IsNearlyZero(CurrentRecoilPitch) && FMath::IsNearlyZero(CurrentRecoilYaw))
	{
		GetWorldTimerManager().ClearTimer(RecoilRecoveryTimer);
	}
}

void AShooterCharacter::UpdateWeaponHUD(int32 CurrentAmmo, int32 MagazineSize)
{
	OnBulletCountUpdated.Broadcast(MagazineSize, CurrentAmmo);
}

FVector AShooterCharacter::GetWeaponTargetLocation()
{
	// 从相机视角向前发射射线
	FHitResult OutHit;

	const FVector Start = GetFirstPersonCameraComponent()->GetComponentLocation();
	const FVector End = Start + (GetFirstPersonCameraComponent()->GetForwardVector() * MaxAimDistance);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);

	GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

	// 返回命中点或检测终点
	return OutHit.bBlockingHit ? OutHit.ImpactPoint : OutHit.TraceEnd;
}

void AShooterCharacter::AddWeaponClass(const TSubclassOf<AShooterWeapon>& WeaponClass)
{
	// 是否已经拥有这把武器？
	AShooterWeapon* OwnedWeapon = FindWeaponOfType(WeaponClass);

	if (!OwnedWeapon)
	{
		// 生成新武器
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = this;
		SpawnParams.Instigator = this;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.TransformScaleMethod = ESpawnActorScaleMethod::MultiplyWithRoot;

		AShooterWeapon* AddedWeapon = GetWorld()->SpawnActor<AShooterWeapon>(WeaponClass, GetActorTransform(), SpawnParams);

		if (AddedWeapon)
		{
			// 将武器添加到拥有列表
			OwnedWeapons.Add(AddedWeapon);

			// 如果已有武器，则停用它
			if (CurrentWeapon)
			{
				CurrentWeapon->DeactivateWeapon();
			}

			// 切换到新武器
			CurrentWeapon = AddedWeapon;
			CurrentWeapon->ActivateWeapon();
		}
	}
}

void AShooterCharacter::OnWeaponActivated(AShooterWeapon* Weapon)
{
	// 更新子弹计数器
	OnBulletCountUpdated.Broadcast(Weapon->GetMagazineSize(), Weapon->GetBulletCount());

	// 设置角色网格的 AnimInstance
	GetFirstPersonMesh()->SetAnimInstanceClass(Weapon->GetFirstPersonAnimInstanceClass());
	GetMesh()->SetAnimInstanceClass(Weapon->GetThirdPersonAnimInstanceClass());
}

void AShooterCharacter::OnWeaponDeactivated(AShooterWeapon* Weapon)
{
	// 未使用
}

void AShooterCharacter::OnSemiWeaponRefire()
{
	// 未使用
}

AShooterWeapon* AShooterCharacter::FindWeaponOfType(TSubclassOf<AShooterWeapon> WeaponClass) const
{
	// 检查每把拥有的武器
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon->IsA(WeaponClass))
		{
			return Weapon;
		}
	}

	// 未找到武器
	return nullptr;

}

void AShooterCharacter::Die()
{
	// 只有服务器能击杀角色
	if (!HasAuthority())
	{
		return;
	}

	// 停用武器并清除增益
	if (IsValid(CurrentWeapon))
	{
		CurrentWeapon->DeactivateWeapon();
		CurrentWeapon->SetDamageMultiplier(1.0f);
	}

	// 清除激活中的增益计时器
	GetWorld()->GetTimerManager().ClearTimer(DamageBoostTimer);
	GetWorld()->GetTimerManager().ClearTimer(ScoreBoostTimer);
	GetWorld()->GetTimerManager().ClearTimer(RecoilRecoveryTimer);
	GetWorld()->GetTimerManager().ClearTimer(RecoilSettleTimer);
	bHasDamageBoost = false;
	bHasScoreBoost = false;

	if (AShooterGameMode* GM = Cast<AShooterGameMode>(GetWorld()->GetAuthGameMode()))
	{
		// 向击杀者发放击杀分数和奖励（AddKillScore 同时处理两者）
		GM->AddKillScore(LastDamageInstigator);

		// 将死亡玩家的分数减半
		GM->OnPlayerDeath(GetController());
	}

	// 为角色添加死亡标签
	Tags.Add(DeathTag);

	// 停止角色移动
	GetCharacterMovement()->StopMovementImmediately();

	// 禁用输入控制
	DisableInput(nullptr);

	// 重置子弹计数器 UI
	OnBulletCountUpdated.Broadcast(0, 0);

	// 调用蓝图处理器
	Multicast_OnDeath();

	// 安排角色重生
	GetWorld()->GetTimerManager().SetTimer(RespawnTimer, this, &AShooterCharacter::OnRespawn, RespawnTime, false);
}

void AShooterCharacter::Multicast_OnDeath_Implementation()
{
	BP_OnDeath();
}

void AShooterCharacter::OnRespawn()
{
	// 销毁角色以强制玩家控制器重生
	Destroy();
}

bool AShooterCharacter::IsDead() const
{
	// 当角色当前 HP 降为零时即视为死亡
	return CurrentHP <= 0.0f;
}

// ---- 增益系统 ----

void AShooterCharacter::GrantKillRewards()
{
	if (!HasAuthority())
	{
		return;
	}

	// 为每把拥有的武器增加一个备用弹匣
	for (AShooterWeapon* Weapon : OwnedWeapons)
	{
		if (Weapon)
		{
			Weapon->AddReserveMagazines(1);
		}
	}

	ShowBuffMessage(TEXT("Kill Reward: +1 Magazine"), FColor::Green);

	// 掷骰判定是否恢复生命
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

	// 掷骰判定是否触发伤害增益
	if (!bHasDamageBoost && FMath::FRand() < DamageBoostChance)
	{
		ActivateDamageBoost();
	}

	// 掷骰判定是否触发分数增益
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
