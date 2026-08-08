// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StateTreeTaskBase.h"
#include "StateTreeConditionBase.h"

#include "ShooterStateTreeUtility.generated.h"

class AShooterNPC;
class AAIController;
class AShooterAIController;

/**
 *  FStateTreeLineOfSightToTargetCondition 条件的实例数据结构
 */
USTRUCT()
struct FStateTreeLineOfSightToTargetConditionInstanceData
{
	GENERATED_BODY()
	
	/** 瞄准方角色 */
	UPROPERTY(EditAnywhere, Category = "Context")
	AShooterNPC* Character;

	/** 需要检查视线的目标 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	AActor* Target;

	/** 允许的最大视线锥角度（度） */
	UPROPERTY(EditAnywhere, Category = "Condition")
	float LineOfSightConeAngle = 35.0f;

	/** 要执行的垂直视线检测次数，用于绕过低矮障碍物 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	int32 NumberOfVerticalLineOfSightChecks = 5;

	/** 如果为 true，角色拥有视线时条件通过 */
	UPROPERTY(EditAnywhere, Category = "Condition")
	bool bMustHaveLineOfSight = true;
};
STATETREE_POD_INSTANCEDATA(FStateTreeLineOfSightToTargetConditionInstanceData);

/**
 *  检查角色是否拥有视线的 StateTree 条件
 */
USTRUCT(DisplayName = "Has Line of Sight to Target", Category="Shooter")
struct FStateTreeLineOfSightToTargetCondition : public FStateTreeConditionCommonBase
{
	GENERATED_BODY()

	/** 设置实例数据类型 */
	using FInstanceDataType = FStateTreeLineOfSightToTargetConditionInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 默认构造函数 */
	FStateTreeLineOfSightToTargetCondition() = default;
	
	/** 测试 StateTree 条件 */
	virtual bool TestCondition(FStateTreeExecutionContext& Context) const override;

#if WITH_EDITOR
	/** 提供描述字符串 */
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif

};

////////////////////////////////////////////////////////////////////

/**
 *  Face Towards Actor StateTree 任务的实例数据结构
 */
USTRUCT()
struct FStateTreeFaceActorInstanceData
{
	GENERATED_BODY()

	/** 决定焦点 Actor 的 AI 控制器 */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller;

	/** 将要面向的 Actor */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> ActorToFaceTowards;
};

/**
 *  让 AI 控制的 Pawn 面向某个 Actor 的 StateTree 任务
 */
USTRUCT(meta=(DisplayName="Face Towards Actor", Category="Shooter"))
struct FStateTreeFaceActorTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* 确保我们使用的是正确的实例数据结构 */
	using FInstanceDataType = FStateTreeFaceActorInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 当所属状态进入时运行 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 当所属状态结束时运行 */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Face Towards Location StateTree 任务的实例数据结构
 */
USTRUCT()
struct FStateTreeFaceLocationInstanceData
{
	GENERATED_BODY()

	/** 决定焦点位置的 AI 控制器 */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller;

	/** 将要面向的位置 */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FVector FaceLocation = FVector::ZeroVector;
};

/**
 *  让 AI 控制的 Pawn 面向某个世界位置的 StateTree 任务
 */
USTRUCT(meta=(DisplayName="Face Towards Location", Category="Shooter"))
struct FStateTreeFaceLocationTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* 确保我们使用的是正确的实例数据结构 */
	using FInstanceDataType = FStateTreeFaceLocationInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 当所属状态进入时运行 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 当所属状态结束时运行 */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Set Random Float StateTree 任务的实例数据结构
 */
USTRUCT()
struct FStateTreeSetRandomFloatData
{
	GENERATED_BODY()

	/** 随机最小值 */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float MinValue = 0.0f;

	/** 随机最大值 */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float MaxValue = 0.0f;

	/** 输出计算值 */
	UPROPERTY(EditAnywhere, Category = Output)
	float OutValue = 0.0f;
};

/**
 *  在指定范围内计算随机浮点值的 StateTree 任务
 */
USTRUCT(meta=(DisplayName="Set Random Float", Category="Shooter"))
struct FStateTreeSetRandomFloatTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* 确保我们使用的是正确的实例数据结构 */
	using FInstanceDataType = FStateTreeSetRandomFloatData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 当所属状态进入时运行 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Shoot At Target StateTree 任务的实例数据结构
 */
USTRUCT()
struct FStateTreeShootAtTargetInstanceData
{
	GENERATED_BODY()

	/** 负责射击的 NPC */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AShooterNPC> Character;

	/** 要射击的目标 */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> Target;
};

/**
 *  让 NPC 向某个 Actor 射击的 StateTree 任务
 */
USTRUCT(meta=(DisplayName="Shoot at Target", Category="Shooter"))
struct FStateTreeShootAtTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* 确保我们使用的是正确的实例数据结构 */
	using FInstanceDataType = FStateTreeShootAtTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 当所属状态进入时运行 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 当所属状态结束时运行 */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Sense Enemies StateTree 任务的实例数据结构
 */
USTRUCT()
struct FStateTreeSenseEnemiesInstanceData
{
	GENERATED_BODY()

	/** 进行感知的 AI 控制器 */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AShooterAIController> Controller;

	/** 进行感知的 NPC */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AShooterNPC> Character;

	/** 感知到的目标 Actor */
	UPROPERTY(EditAnywhere, Category = Output)
	TObjectPtr<AActor> TargetActor;

	/** 感知到的调查位置 */
	UPROPERTY(EditAnywhere, Category = Output)
	FVector InvestigateLocation = FVector::ZeroVector;

	/** 如果成功感知到目标则为 true */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasTarget = false;

	/** 如果成功感知到调查位置则为 true */
	UPROPERTY(EditAnywhere, Category = Output)
	bool bHasInvestigateLocation = false;

	/** 感知 Actor 上必须携带的标签 */
	UPROPERTY(EditAnywhere, Category = Parameter)
	FName SenseTag = FName("Player");

	/** 视为完整感知的视线锥半角 */
	UPROPERTY(EditAnywhere, Category = Parameter)
	float DirectLineOfSightCone = 85.0f;

	/** 最后处理的刺激强度 */
	UPROPERTY(EditAnywhere)
	float LastStimulusStrength = 0.0f;
};

/**
 *  让 NPC 处理 AI 感知并感知附近敌人的 StateTree 任务
 */
USTRUCT(meta=(DisplayName="Sense Enemies", Category="Shooter"))
struct FStateTreeSenseEnemiesTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* 确保我们使用的是正确的实例数据结构 */
	using FInstanceDataType = FStateTreeSenseEnemiesInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 当所属状态进入时运行 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 当所属状态结束时运行 */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};

////////////////////////////////////////////////////////////////////

/**
 *  Move To Target StateTree 任务的实例数据结构
 */
USTRUCT()
struct FStateTreeMoveToTargetInstanceData
{
	GENERATED_BODY()

	/** 负责移动的 AI 控制器 */
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AAIController> Controller;

	/** 要移动到的目标 Actor（优先于 TargetLocation） */
	UPROPERTY(EditAnywhere, Category = Input)
	TObjectPtr<AActor> TargetActor;

	/** 要移动到的目标位置（仅在 TargetActor 为空时使用） */
	UPROPERTY(EditAnywhere, Category = Input)
	FVector TargetLocation = FVector::ZeroVector;

	/** 移动被视为完成的距离 */
	UPROPERTY(EditAnywhere, Category = Parameter, meta = (ClampMin = 0, Units = "cm"))
	float AcceptableRadius = 200.0f;
};

/**
 *  让 AI 控制的 Pawn 向目标 Actor 或位置移动的 StateTree 任务
 */
USTRUCT(meta=(DisplayName="Move To Target", Category="Shooter"))
struct FStateTreeMoveToTargetTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	/* 确保我们使用的是正确的实例数据结构 */
	using FInstanceDataType = FStateTreeMoveToTargetInstanceData;
	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	/** 当所属状态进入时运行 */
	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

	/** 当所属状态结束时运行 */
	virtual void ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const override;

#if WITH_EDITOR
	virtual FText GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting = EStateTreeNodeFormatting::Text) const override;
#endif // WITH_EDITOR
};
