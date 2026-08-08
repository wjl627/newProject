// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "ShooterAIController.generated.h"

class UAIPerceptionComponent;
class AShooterNPC;
class UBehaviorTree;
struct FAIStimulus;

DECLARE_DELEGATE_TwoParams(FShooterPerceptionUpdatedDelegate, AActor*, const FAIStimulus&);
DECLARE_DELEGATE_OneParam(FShooterPerceptionForgottenDelegate, AActor*);

// -----------------------------------------------------------------------------
// 状态机——已弃用，仅作参考保留
// -----------------------------------------------------------------------------
#if 0

UENUM()
enum class EAIState : uint8
{
	Patrol,
	Chase,
	Attack
};

#endif // 状态机

UCLASS(abstract)
class NEWPROJECT_API AShooterAIController : public AAIController
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UAIPerceptionComponent* AIPerception;

protected:

	UPROPERTY(EditAnywhere, Category="Shooter")
	FName TeamTag = FName("Enemy");

	UPROPERTY(EditAnywhere, Category="Shooter|Perception")
	FName SenseTag = FName("Player");

	/** 运行时应用到感知组件的视觉半径（覆盖蓝图配置） */
	UPROPERTY(EditAnywhere, Category="Shooter|Perception", meta = (ClampMin = 0, Units = "cm"))
	float PerceptionSightRadius = 2500.0f;

	/** 目标离开视线后仍被记住的半径范围 */
	UPROPERTY(EditAnywhere, Category="Shooter|Perception", meta = (ClampMin = 0, Units = "cm"))
	float PerceptionLoseSightRadius = 3200.0f;

	/** 视觉感知的外围视野角度（度） */
	UPROPERTY(EditAnywhere, Category="Shooter|Perception", meta = (ClampMin = 1, ClampMax = 180, Units = "deg"))
	float PerceptionSightAngle = 110.0f;

	/** 目标离开视线后仍被记住的时长 */
	UPROPERTY(EditAnywhere, Category="Shooter|Perception", meta = (ClampMin = 0.1f, ClampMax = 30, Units = "s"))
	float PerceptionMaxAge = 8.0f;

	/** 此半径内的目标无论是否在视线锥内都会被检测到（防止“太近而无法攻击”） */
	UPROPERTY(EditAnywhere, Category="Shooter|Perception", meta = (ClampMin = 0, Units = "cm"))
	float CloseRangeAutoDetectRadius = 500.0f;

	// --- 巡逻 ---

	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, Units = "cm"))
	float PatrolRadius = 1500.0f;

#if 0 // State Machine
	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float MinPatrolWait = 1.0f;

	UPROPERTY(EditAnywhere, Category="Shooter|Patrol", meta = (ClampMin = 0, ClampMax = 10, Units = "s"))
	float MaxPatrolWait = 3.0f;
#endif

	// --- 战斗 ---

	UPROPERTY(EditAnywhere, Category="Shooter|Combat", meta = (ClampMin = 0, Units = "cm"))
	float AttackRange = 2000.0f;

	UPROPERTY(EditAnywhere, Category="Shooter|Combat", meta = (ClampMin = 0, Units = "cm"))
	float LoseTargetRange = 4000.0f;

	// --- 运行时 ---

	TObjectPtr<AActor> TargetEnemy;

	TObjectPtr<AShooterNPC> PossessedNPC;

	/** 最后一次已知的目标位置，用于失去视野后追击/调查 */
	FVector LastKnownTargetLocation = FVector::ZeroVector;

	/** 目标最后一次被看到（或被记住）时的游戏时间 */
	float LastTargetSeenTime = -1000.0f;

#if 0 // State Machine
	EAIState CurrentState = EAIState::Patrol;
	FTimerHandle PatrolWaitTimer;
	bool bIsWaitingAtPatrol = false;
#endif

public:

	AShooterAIController();

	FShooterPerceptionUpdatedDelegate OnShooterPerceptionUpdated;
	FShooterPerceptionForgottenDelegate OnShooterPerceptionForgotten;

	void SetCurrentTarget(AActor* Target) { TargetEnemy = Target; }
	void ClearCurrentTarget() { TargetEnemy = nullptr; }
	AActor* GetCurrentTarget() const { return TargetEnemy; }

	// --- 供行为树节点使用的 Getter ---

	FName GetSenseTag() const { return SenseTag; }
	float GetAttackRange() const { return AttackRange; }
	float GetLoseTargetRange() const { return LoseTargetRange; }
	float GetPatrolRadius() const { return PatrolRadius; }
	bool HasLineOfSightToTarget() const;
	float GetCloseRangeAutoDetectRadius() const { return CloseRangeAutoDetectRadius; }
	float GetPerceptionMaxAge() const { return PerceptionMaxAge; }

	/** 记录当前目标位置并刷新最后一次看到目标的时间 */
	void UpdateLastKnownTargetLocation(const FVector& Location);

	/** 如果目标在 MaxAge 秒内被看到过，则返回最后一次已知的目标位置 */
	bool GetLastKnownTargetLocation(FVector& OutLocation, float MaxAge) const;

	/** 自目标最后一次被看到/记住以来经过的时间 */
	float GetTimeSinceLastSeenTarget() const;

	// --- 行为树 ---

	UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	bool bUseBehaviorTree = true;

	UPROPERTY(EditAnywhere, Category = "AI|BehaviorTree")
	TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

protected:

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaTime) override;

#if 0 // 状态机
	void SetState(EAIState NewState);
	void TickPatrol(float DeltaTime);
	void TickChase(float DeltaTime);
	void TickAttack(float DeltaTime);
	float GetDistanceToTarget() const;
	FVector GetRandomPatrolPoint() const;
#endif

	// --- 感知 ---

	UFUNCTION()
	void OnPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	UFUNCTION()
	void OnPerceptionForgotten(AActor* Actor);

	// --- 死亡 ---

	UFUNCTION()
	void OnPawnDeath();
};
