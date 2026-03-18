// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AutoPickUpEffectActor.h"

#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/ProjectileMovementComponent.h"

AAutoPickUpEffectActor::AAutoPickUpEffectActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// - Create Movement Component - //
	// ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("MovementComponent");
	// ProjectileMovement->SetUpdatedComponent(RootComponent);
	// Initial
	
	FloatAmplitude = 20.f;
	FloatSpeed = 2.f;

	AttractSpeed = 8.f;

	bStartAttract = false;
	TargetPlayer = nullptr;
}

void AAutoPickUpEffectActor::BeginPlay()
{
	Super::BeginPlay();
	SetActorTickEnabled(false);
	StartLocation = GetActorLocation();
	
	GetWorldTimerManager().SetTimer(
		UpdateTimer,
		this,
		&AAutoPickUpEffectActor::UpdateItem,
		0.02f,
		true
	);
}

void AAutoPickUpEffectActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}



void AAutoPickUpEffectActor::StartAttract(AActor* InTargetActor)
{
	if (!InTargetActor)
	{
		return;
	}
	if (!InTargetActor->ActorHasTag("Player"))
	{
		return;
	}
	TargetPlayer = InTargetActor;
	bStartAttract = true;
}

void AAutoPickUpEffectActor::UpdateItem()
{
	const float Time = GetWorld()->GetTimeSeconds();
	const FVector CurrentLocation = GetActorLocation();
	const float OffsetZ = FMath::Sin(Time * FloatSpeed) * FloatAmplitude;

	FVector FloatLocation = StartLocation;
	FloatLocation.Z += OffsetZ;

	if (bStartAttract && TargetPlayer)
	{
		const FVector TargetLocation = TargetPlayer->GetActorLocation();

		FloatLocation = FMath::VInterpTo(
			CurrentLocation,
			TargetLocation,
			0.02f,
			AttractSpeed
		);
	}

	SetActorLocation(FloatLocation);
	AddActorLocalRotation(FRotator(0, 2.f, 0));
}
