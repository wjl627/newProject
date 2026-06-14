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

	/** Scale along the forward axis to make the bullet look like a streak */
	UPROPERTY(EditAnywhere, Category="Tracer")
	float StreakScale = 6.0f;

	/** The axis of the mesh that points forward along the flight path */
	UPROPERTY(EditAnywhere, Category="Tracer")
	ETracerForwardAxis ForwardAxis = ETracerForwardAxis::ZAxis;

	FVector StartLocation;
	FVector EndLocation;
	float Age = 0.0f;
};
