// Fill out your copyright notice in the Description page of Project Settings.

#include "Loot/USnDropMotionComponent.h"

#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"

USnDropMotionComponent::USnDropMotionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// The initial velocity is replicated so client copies replay the same arc
	// (component property replication - the pickup actors are SetReplicates).
	SetIsReplicatedByDefault(true);
}

void USnDropMotionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// The launch replicates as a PAIR - where it started and how fast it left.
	// Clients need both to replay the arc from the right place: the start point
	// is not otherwise knowable, because a pooled pickup's teleport to its new
	// drop spot happens server-side only (see OnRep_DropInitialVelocity).
	DOREPLIFETIME(USnDropMotionComponent, DropStartLocation);
	DOREPLIFETIME(USnDropMotionComponent, DropInitialVelocity);
}

void USnDropMotionComponent::LaunchDrop()
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}

	// Server only: records the start point, rolls the random scatter, then
	// shares the whole launch with clients (LaunchDropWithVelocity runs on both).
	if (Owner->GetLocalRole() == ROLE_Authority)
	{
		// Half of the replicated launch. The owner's location is the truth here
		// - a pooled pickup was just teleported to its new drop spot - and the
		// client copy has no other way to learn it (no movement replication).
		DropStartLocation = Owner->GetActorLocation();

		// Pop out: upward impulse plus a random sideways nudge so drops scatter
		// instead of stacking on one spot. The arc is simulated in code (no
		// rigid body) - deterministic landing, no physics tunneling.
		const FVector ScatteredVelocity(
			FMath::FRandRange(-HorizontalScatter, HorizontalScatter),
			FMath::FRandRange(-HorizontalScatter, HorizontalScatter),
			SpawnImpulse);
		DropInitialVelocity = ScatteredVelocity;
		LaunchDropWithVelocity(ScatteredVelocity);
	}
}

void USnDropMotionComponent::LaunchDropWithVelocity(const FVector& InVelocity)
{
	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}

	BallisticVelocity = InVelocity;
	BounceCount = 0;
	bSettled = false;
	bLaunched = true;

	// Fallback: force floating if the owner never lands (e.g. dropped off a ledge).
	GetWorld()->GetTimerManager().SetTimer(SettleTimerHandle, this, &USnDropMotionComponent::HandleBallisticTimeout, BallisticTimeout, false);
}

void USnDropMotionComponent::OnRep_DropInitialVelocity()
{
	// The server rolled this drop's arc; replay it locally so the client copy
	// moves with the same trajectory (client-side simulation - the replicated
	// transform is not used for the visual motion).
	//
	// Snap to the replicated start point FIRST: a reused (pooled) pickup was
	// teleported to its new drop spot on the server, that move never replicates,
	// and this copy is still sitting where the PREVIOUS drop landed. Launching
	// from there would put the host's and the client's item in two different
	// places. RepNotifies run after the whole bunch is applied, so
	// DropStartLocation is already in hand.
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(DropStartLocation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	}
	LaunchDropWithVelocity(DropInitialVelocity);
}

void USnDropMotionComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* Owner = GetOwner();
	if (!Owner || !GetWorld())
	{
		return;
	}
	// Not launched yet: the client copy waits for its replicated initial
	// velocity (it stays where it spawned until the arc is known).
	if (!bLaunched)
	{
		return;
	}

	if (bSettled)
	{
		// Idle float: spin + gentle bob around the settled location.
		FloatPhase += DeltaTime;
		Owner->SetActorRotation(Owner->GetActorRotation() + FRotator(0.f, FloatRotateSpeed * DeltaTime, 0.f));
		const FVector Bobbed = SettledLocation + FVector(0.f, 0.f, FMath::Sin(FloatPhase * FloatBobSpeed) * FloatBobHeight);
		Owner->SetActorLocation(Bobbed);
		return;
	}

	// Arc: integrate gravity, then check for the ground below.
	BallisticVelocity.Z -= Gravity * DeltaTime;
	FVector NewLocation = Owner->GetActorLocation() + BallisticVelocity * DeltaTime;

	FHitResult Hit;
	FCollisionQueryParams Params(FName(TEXT("SnDropLand")), /*bTraceComplex=*/true);
	GetWorld()->LineTraceSingleByChannel(
		Hit,
		NewLocation,
		NewLocation + FVector(0.f, 0.f, -TraceProbeDepth),
		ECC_WorldStatic,
		Params);

	if (Hit.bBlockingHit)
	{
		// Landed. One soft bounce, then settle into the float animation.
		NewLocation = Hit.ImpactPoint + FVector(0.f, 0.f, LandLiftHeight);
		if (BounceCount == 0 && BallisticVelocity.Z < BounceMinSpeed)
		{
			BallisticVelocity.Z = -BallisticVelocity.Z * BounceRestitution;
			BallisticVelocity.X *= 0.5f;
			BallisticVelocity.Y *= 0.5f;
			++BounceCount;
		}
		else
		{
			GetWorld()->GetTimerManager().ClearTimer(SettleTimerHandle);
			EnterFloatMode(NewLocation);
			return;
		}
	}

	Owner->SetActorLocation(NewLocation);
}

void USnDropMotionComponent::HandleBallisticTimeout()
{
	// Never landed (fell into a gap): settle where it is.
	if (AActor* Owner = GetOwner())
	{
		EnterFloatMode(Owner->GetActorLocation());
	}
}

void USnDropMotionComponent::EnterFloatMode(const FVector& InSettledLocation)
{
	bSettled = true;
	SettledLocation = InSettledLocation;
	if (AActor* Owner = GetOwner())
	{
		Owner->SetActorLocation(InSettledLocation);
	}
	FloatPhase = 0.f;
}
