// Copyright Epic Games, Inc. All Rights Reserved.

#include "ShooterBulletTracer.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

AShooterBulletTracer::AShooterBulletTracer()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = false;

	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	RootComponent = BulletMesh;
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMesh->SetCastShadow(false);
}

void AShooterBulletTracer::Initialize(const FVector& InStart, const FVector& InEnd)
{
	StartLocation = InStart;
	EndLocation = InEnd;
	SetActorLocation(InStart);
	Age = 0.0f;

	const FVector Dir = (InEnd - InStart).GetSafeNormal();
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
}

void AShooterBulletTracer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

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
