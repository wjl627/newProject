// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ShooterBulletTracer.generated.h"

class UStaticMeshComponent;

UENUM()
enum class ETracerForwardAxis : uint8
{
	XAxis,
	YAxis,
	ZAxis
};

UCLASS()
class NEWPROJECT_API AShooterBulletTracer : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Components", meta = (AllowPrivateAccess = "true"))
	UStaticMeshComponent* BulletMesh;

public:

	AShooterBulletTracer();

	void Initialize(const FVector& InStart, const FVector& InEnd);

protected:

	virtual void Tick(float DeltaTime) override;

	UPROPERTY(EditAnywhere, Category="Tracer")
	float Speed = 10000.0f;

	UPROPERTY(EditAnywhere, Category="Tracer")
	float MaxLifetime = 2.0f;

	/** 沿前向轴的缩放，让子弹看起来像一条拖尾 */
	UPROPERTY(EditAnywhere, Category="Tracer")
	float StreakScale = 6.0f;

	/** 网格沿前向轴的半长度，用于补偿枢轴不在端点的情况。设置为网格长度的一半左右 */
	UPROPERTY(EditAnywhere, Category="Tracer")
	float MeshPivotOffset = 0.0f;

	/** 网格中沿飞行路径朝前的轴 */
	UPROPERTY(EditAnywhere, Category="Tracer")
	ETracerForwardAxis ForwardAxis = ETracerForwardAxis::ZAxis;

	FVector StartLocation;
	FVector EndLocation;
	float Age = 0.0f;

	/** 跳过第一次 Tick，避免多人游戏中网络接收生成时同帧移动 */
	bool bSkipFirstTick = true;
};
