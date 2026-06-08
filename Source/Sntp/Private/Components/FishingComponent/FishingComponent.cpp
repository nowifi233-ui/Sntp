// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/FishingComponent/FishingComponent.h"

// Sets default values for this component's properties
UFishingComponent::UFishingComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UFishingComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
}


// Called every frame
void UFishingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
	if (FishingState != EFishingState::Fighting)
	{
		return;
	}
	UpdateBar(DeltaTime);
	UpdateFish(DeltaTime);
	UpdateCatchProgress(DeltaTime);
	
	OnFishingUIUpdate.Broadcast(FishPosition, BarPosition, CatchProgress);
}

void UFishingComponent::StartFishing()
{
	FishingState = EFishingState::Fighting;
	
	BarPosition = 0.f;
	BarVelocity = 0.f;
	
	FishTargetPosition = 0.5f;
	FishPosition = 0.f;
	
	CatchProgress = 0.1f;
}

void UFishingComponent::StopFishing()
{
	FishingState = EFishingState::Idle;
}

void UFishingComponent::SetInputPressed(bool bPressed)
{
	bHoldingInput = bPressed;
}

void UFishingComponent::UpdateBar(float DeltaTime)
{
	float Acc = Gravity;
	if (bHoldingInput)
	{
		Acc += InputForce;
	}
	
	BarVelocity += Acc * DeltaTime;
	
	// Bar
	
	BarPosition += BarVelocity * DeltaTime;
	
	BarPosition = FMath::Clamp(BarPosition, 0.f, 1.f - BarHeight);
	
	if (BarPosition <= 0.f || BarPosition >= 1.f - BarHeight)
	{
		BarVelocity = -0.5f * BarVelocity;
	}
}

void UFishingComponent::UpdateFish(float DeltaTime)
{
	FishMoveTimer -= DeltaTime;
	if (FishMoveTimer <= 0.f)
	{
		FishMoveTimer = FishMoveInterval;
		FishTargetPosition = FMath::FRandRange(0.f, 1.f);
	}
	FishPosition = FMath::FInterpTo(
		FishPosition,
		FishTargetPosition,
		DeltaTime,
		FishSpeed
		);
	
	OnFishDebug.Broadcast(FishTargetPosition, FishSpeed, FishMoveTimer);
}

void UFishingComponent::UpdateCatchProgress(float DeltaTime)
{
	if (IsFishInsideBar())
	{
		CatchProgress += CatchIncreaseSpeed * DeltaTime;
	}
	else
	{
		CatchProgress -= CatchDecreaseSpeed * DeltaTime;
	}
	
	CatchProgress = FMath::Clamp(CatchProgress, 0.f, 1.f);
	if (CatchProgress >= 1.f)
	{
		FishingState = EFishingState::Success;
	}
	if (CatchProgress < 0.f)
	{
		FishingState = EFishingState::Failed;
	}
}

bool UFishingComponent::IsFishInsideBar() const
{
	const float BarTop = BarPosition + BarHeight;
	const float BarBottom = BarPosition;
	return FishPosition <= BarTop && FishPosition >= BarBottom;
}

