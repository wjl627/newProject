// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterBulletTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

	// Local-space bounds so we can align the mesh's rear end (not its pivot) to the muzzle
	FBox LocalBox(ForceInit);
	if (const UStaticMesh* Mesh = BulletMesh->GetStaticMesh())
	{
		LocalBox = Mesh->GetBoundingBox();
	}

	FVector Scale = FVector::OneVector;
	float NearEnd = 0.0f; // most-negative extent of the mesh along its forward axis, in local space

	switch (ForwardAxis)
	{
	case ETracerForwardAxis::XAxis:
		SetActorRotation(Dir.Rotation());
		Scale.X = StreakScale;
		NearEnd = LocalBox.Min.X;
		break;
	case ETracerForwardAxis::YAxis:
		SetActorRotation(UKismetMathLibrary::MakeRotFromY(Dir));
		Scale.Y = StreakScale;
		NearEnd = LocalBox.Min.Y;
		break;
	case ETracerForwardAxis::ZAxis:
		SetActorRotation(UKismetMathLibrary::MakeRotFromZ(Dir));
		Scale.Z = StreakScale;
		NearEnd = LocalBox.Min.Z;
		break;
	}

	BulletMesh->SetWorldScale3D(Scale);

	// Push the pivot forward so the mesh's rear end sits at the muzzle rather than its center.
	// NearEnd is usually negative (pivot near center), so this places the whole streak in front of the muzzle.
	const float TailOffset = (-NearEnd * StreakScale) + MeshPivotOffset;
	StartLocation = InStart + Dir * TailOffset;
	EndLocation = InEnd + Dir * TailOffset;

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
