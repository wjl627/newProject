// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterBulletTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

AShooterBulletTracer::AShooterBulletTracer()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = false;

	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	RootComponent = BulletMesh;
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMesh->SetCastShadow(false);
}

void AShooterBulletTracer::Initialize(const FVector& InStart, const FVector& InEnd)
{
	Age = 0.0f;
	bSkipFirstTick = true;

	const FVector Dir = (InEnd - InStart).GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		Destroy();
		return;
	}

	const FVector MeshOffsetVec = Dir * MeshPivotOffset;
	StartLocation = InStart - MeshOffsetVec;
	EndLocation = InEnd - MeshOffsetVec;

	FVector Scale = FVector::OneVector;

	switch (ForwardAxis)
	{
	case ETracerForwardAxis::XAxis:
		SetActorRotation(Dir.Rotation());
		Scale.X = StreakScale;
		break;
	case ETracerForwardAxis::YAxis:
		SetActorRotation(UKismetMathLibrary::MakeRotFromY(Dir));
		Scale.Y = StreakScale;
		break;
	case ETracerForwardAxis::ZAxis:
		SetActorRotation(UKismetMathLibrary::MakeRotFromZ(Dir));
		Scale.Z = StreakScale;
		break;
	}

	BulletMesh->SetWorldScale3D(Scale);
	SetActorLocation(StartLocation);
	SetActorTickEnabled(true);
}

void AShooterBulletTracer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bSkipFirstTick)
	{
		bSkipFirstTick = false;
		return;
	}

	Age += DeltaTime;

	const FVector Dir = (EndLocation - StartLocation).GetSafeNormal();
	const FVector CurrentPos = StartLocation + Dir * Speed * Age;

	if (Age >= MaxLifetime || FVector::DotProduct(CurrentPos - EndLocation, Dir) >= 0.0f)
	{
		Destroy();
		return;
	}

	SetActorLocation(CurrentPos);
}
