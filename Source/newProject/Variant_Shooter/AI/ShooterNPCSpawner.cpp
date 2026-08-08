// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/AI/ShooterNPCSpawner.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/ArrowComponent.h"
#include "TimerManager.h"
#include "ShooterNPC.h"

// 设置默认值
AShooterNPCSpawner::AShooterNPCSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// 创建根组件
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// 创建参考生成胶囊体
	SpawnCapsule = CreateDefaultSubobject<UCapsuleComponent>(TEXT("Spawn Capsule"));
	SpawnCapsule->SetupAttachment(RootComponent);

	SpawnCapsule->SetRelativeLocation(FVector(0.0f, 0.0f, 90.0f));
	SpawnCapsule->SetCapsuleSize(35.0f, 90.0f);
	SpawnCapsule->SetCollisionProfileName(FName("NoCollision"));

	SpawnDirection = CreateDefaultSubobject<UArrowComponent>(TEXT("Spawn Direction"));
	SpawnDirection->SetupAttachment(RootComponent);
}

void AShooterNPCSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	// 仅在启用且设置了生成数量时自动生成
	if (bAutoSpawn && SpawnCount > 0)
	{
		// 安排第一个 NPC 的生成
		GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterNPCSpawner::SpawnNPC, InitialSpawnDelay);
	}
}

void AShooterNPCSpawner::EndPlay(EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	// 清除生成计时器
	GetWorld()->GetTimerManager().ClearTimer(SpawnTimer);
}

void AShooterNPCSpawner::SpawnNPC()
{
	// 确保 NPC 类有效
	if (IsValid(NPCClass))
	{
		// 在参考胶囊体的变换处生成 NPC
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		AShooterNPC* SpawnedNPC = GetWorld()->SpawnActor<AShooterNPC>(NPCClass, SpawnCapsule->GetComponentTransform(), SpawnParams);

		// NPC 是否创建成功？
		if (SpawnedNPC)
		{
			// 订阅死亡委托
			SpawnedNPC->OnPawnDeath.AddDynamic(this, &AShooterNPCSpawner::OnNPCDied);
		}
	}
}

FTransform AShooterNPCSpawner::GetSpawnTransform() const
{
	return SpawnCapsule->GetComponentTransform();
}

void AShooterNPCSpawner::OnNPCDied()
{
	// 减少生成计数器
	--SpawnCount;

	// 这是否是我们要生成的最后一个 NPC？
	if (SpawnCount <= 0)
	{
		return;
	}

	// 安排下一个 NPC 的生成
	GetWorld()->GetTimerManager().SetTimer(SpawnTimer, this, &AShooterNPCSpawner::SpawnNPC, RespawnDelay);
}
