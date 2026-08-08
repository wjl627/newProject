// Copyright Epic Games, Inc. All Rights Reserved.


#include "Variant_Shooter/AI/ShooterStateTreeUtility.h"
#include "StateTreeExecutionContext.h"
#include "ShooterNPC.h"
#include "Camera/CameraComponent.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "ShooterAIController.h"
#include "StateTreeAsyncExecutionContext.h"

bool FStateTreeLineOfSightToTargetCondition::TestCondition(FStateTreeExecutionContext& Context) const
{
	const FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

	// 确保目标有效
	if (!IsValid(InstanceData.Target))
	{
		return !InstanceData.bMustHaveLineOfSight;
	}
	
	// 检查角色是否正朝向目标
	const FVector TargetDir = (InstanceData.Target->GetActorLocation() - InstanceData.Character->GetActorLocation()).GetSafeNormal();

	const float FacingDot = FVector::DotProduct(TargetDir, InstanceData.Character->GetActorForwardVector());
	const float MaxDot = FMath::Cos(FMath::DegreesToRadians(InstanceData.LineOfSightConeAngle));

	// 朝向是否超出我们的锥形半角范围？
	if (FacingDot <= MaxDot)
	{
		return !InstanceData.bMustHaveLineOfSight;
	}

	// 获取目标的包围盒
	FVector CenterOfMass, Extent;
	InstanceData.Target->GetActorBounds(true, CenterOfMass, Extent, false);

	// 将垂直范围除以将要执行的视线检测次数
	const float ExtentZOffset = Extent.Z * 2.0f / InstanceData.NumberOfVerticalLineOfSightChecks;

	// 以角色相机位置作为视线检测的起点
	const FVector Start = InstanceData.Character->GetFirstPersonCameraComponent()->GetComponentLocation();

	// 忽略角色和目标本身，确保检测到的是不包含它们的无遮挡射线
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(InstanceData.Character);
	QueryParams.AddIgnoredActor(InstanceData.Target);

	FHitResult OutHit;

	// 向目标位置发射多条带垂直偏移的射线
	for (int32 i = 0; i < InstanceData.NumberOfVerticalLineOfSightChecks - 1; ++i)
	{
		// 计算射线的终点
		const FVector End = CenterOfMass + FVector(0.0f, 0.0f, Extent.Z - ExtentZOffset * i);

		InstanceData.Character->GetWorld()->LineTraceSingleByChannel(OutHit, Start, End, ECC_Visibility, QueryParams);

		// 射线是否无遮挡？
		if (!OutHit.bBlockingHit)
		{
			// 只需要一条无遮挡射线，因此提前结束
			return InstanceData.bMustHaveLineOfSight;
		}
	}

	// 未找到视线
	return !InstanceData.bMustHaveLineOfSight;
}

#if WITH_EDITOR
FText FStateTreeLineOfSightToTargetCondition::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Has Line of Sight</b>");
}
#endif

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeFaceActorTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否从其他状态转换过来？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 设置 AI 控制器的焦点
		InstanceData.Controller->SetFocus(InstanceData.ActorToFaceTowards);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceActorTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否已转换到其他状态？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 清除 AI 控制器的焦点
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceActorTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Face Towards Actor</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeFaceLocationTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否从其他状态转换过来？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 设置 AI 控制器的焦点
		InstanceData.Controller->SetFocalPoint(InstanceData.FaceLocation);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeFaceLocationTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否已转换到其他状态？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 清除 AI 控制器的焦点
		InstanceData.Controller->ClearFocus(EAIFocusPriority::Gameplay);
	}
}

#if WITH_EDITOR
FText FStateTreeFaceLocationTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Face Towards Location</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeSetRandomFloatTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否已转换到其他状态？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 计算输出值
		InstanceData.OutValue = FMath::RandRange(InstanceData.MinValue, InstanceData.MaxValue);
	}

	return EStateTreeRunStatus::Running;
}

#if WITH_EDITOR
FText FStateTreeSetRandomFloatTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Set Random Float</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeShootAtTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否从其他状态转换过来？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 让角色向目标射击
		InstanceData.Character->StartShooting(InstanceData.Target);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeShootAtTargetTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否已转换到其他状态？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 让角色停止射击
		InstanceData.Character->StopShooting();
	}
}

#if WITH_EDITOR
FText FStateTreeShootAtTargetTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Shoot at Target</b>");
}
#endif // WITH_EDITOR

EStateTreeRunStatus FStateTreeSenseEnemiesTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否从其他状态转换过来？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 在控制器上绑定感知更新委托
		InstanceData.Controller->OnShooterPerceptionUpdated.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](AActor* SensedActor, const FAIStimulus& Stimulus)
			{
				// 在 lambda 内部获取实例数据
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();
				if (FInstanceDataType* LambdaInstanceData = StrongContext.GetInstanceDataPtr<FInstanceDataType>())
				{
					if (SensedActor->ActorHasTag(LambdaInstanceData->SenseTag))
					{
						bool bDirectLOS = false;

						// 计算刺激的方向
						const FVector StimulusDir = (Stimulus.StimulusLocation - LambdaInstanceData->Character->GetActorLocation()).GetSafeNormal();

						// 通过角色朝向与刺激方向的点积推断角度
						const float DirDot = FVector::DotProduct(StimulusDir, LambdaInstanceData->Character->GetActorForwardVector());
						const float MaxDot = FMath::Cos(FMath::DegreesToRadians(LambdaInstanceData->DirectLineOfSightCone));

						// 该方向是否在我们的感知锥内？
						if (DirDot >= MaxDot)
						{
							// 在角色与感知到的 Actor 之间执行射线检测
							FCollisionQueryParams QueryParams;
							QueryParams.AddIgnoredActor(LambdaInstanceData->Character);
							QueryParams.AddIgnoredActor(SensedActor);

							FHitResult OutHit;

							// 如果射线无遮挡，则拥有直接视线
							bDirectLOS = !LambdaInstanceData->Character->GetWorld()->LineTraceSingleByChannel(OutHit, LambdaInstanceData->Character->GetActorLocation(), SensedActor->GetActorLocation(), ECC_Visibility, QueryParams);

						}

						// 检查我们是否对刺激来源有直接视线
						if (bDirectLOS)
						{
							// 设置控制器的目标
							LambdaInstanceData->Controller->SetCurrentTarget(SensedActor);

							// 设置任务输出
							LambdaInstanceData->TargetActor = SensedActor;

							// 设置标记
							LambdaInstanceData->bHasTarget = true;
							LambdaInstanceData->bHasInvestigateLocation = false;

						// 对目标没有直接视线
						} else {

							// 如果已有目标，则忽略部分感知并继续跟踪
							if (!IsValid(LambdaInstanceData->TargetActor))
							{
								// 这个刺激是否比上一次更强？
								if (Stimulus.Strength > LambdaInstanceData->LastStimulusStrength)
								{
									// 更新刺激强度
									LambdaInstanceData->LastStimulusStrength = Stimulus.Strength;

									// 设置调查位置
									LambdaInstanceData->InvestigateLocation = Stimulus.StimulusLocation;

									// 设置调查标记
									LambdaInstanceData->bHasInvestigateLocation = true;
								}
							}
						}
					}
				}
			}
		);

		// 在控制器上绑定感知遗忘委托
		InstanceData.Controller->OnShooterPerceptionForgotten.BindLambda(
			[WeakContext = Context.MakeWeakExecutionContext()](AActor* SensedActor)
			{
				// 在 lambda 内部获取实例数据
				const FStateTreeStrongExecutionContext StrongContext = WeakContext.MakeStrongExecutionContext();
				if (FInstanceDataType* LambdaInstanceData = StrongContext.GetInstanceDataPtr<FInstanceDataType>())
				{
					bool bForget = false;

					// 是否正在遗忘当前目标？
					if (SensedActor == LambdaInstanceData->TargetActor)
					{
						bForget = true;
					}
					else 
					{
						// 是否正在遗忘部分感知？
						if (!IsValid(LambdaInstanceData->TargetActor))
						{
							bForget = true;
						}
					}

					if (bForget)
					{
						// 清除目标
						LambdaInstanceData->TargetActor = nullptr;

						// 清除标记
						LambdaInstanceData->bHasInvestigateLocation = false;
						LambdaInstanceData->bHasTarget = false;

						// 重置刺激强度
						LambdaInstanceData->LastStimulusStrength = 0.0f;

						// 清除控制器上的目标
						LambdaInstanceData->Controller->ClearCurrentTarget();
						LambdaInstanceData->Controller->ClearFocus(EAIFocusPriority::Gameplay);
					}
				}
			}
		);
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeSenseEnemiesTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否已转换到其他状态？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 解绑感知委托
		InstanceData.Controller->OnShooterPerceptionUpdated.Unbind();
		InstanceData.Controller->OnShooterPerceptionForgotten.Unbind();
	}
}

#if WITH_EDITOR
FText FStateTreeSenseEnemiesTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Sense Enemies</b>");
}
#endif // WITH_EDITOR

////////////////////////////////////////////////////////////////////

EStateTreeRunStatus FStateTreeMoveToTargetTask::EnterState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否从其他状态转换过来？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		if (IsValid(InstanceData.TargetActor))
		{
			InstanceData.Controller->MoveToActor(InstanceData.TargetActor, InstanceData.AcceptableRadius);
		}
		else
		{
			InstanceData.Controller->MoveToLocation(InstanceData.TargetLocation);
		}
	}

	return EStateTreeRunStatus::Running;
}

void FStateTreeMoveToTargetTask::ExitState(FStateTreeExecutionContext& Context, const FStateTreeTransitionResult& Transition) const
{
	// 是否已转换到其他状态？
	if (Transition.ChangeType == EStateTreeStateChangeType::Changed)
	{
		// 获取实例数据
		FInstanceDataType& InstanceData = Context.GetInstanceData(*this);

		// 中止移动
		InstanceData.Controller->StopMovement();
	}
}

#if WITH_EDITOR
FText FStateTreeMoveToTargetTask::GetDescription(const FGuid& ID, FStateTreeDataView InstanceDataView, const IStateTreeBindingLookup& BindingLookup, EStateTreeNodeFormatting Formatting /*= EStateTreeNodeFormatting::Text*/) const
{
	return FText::FromString("<b>Move To Target</b>");
}
#endif // WITH_EDITOR
