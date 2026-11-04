// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/USnBuildLibrary.h"

#include "Engine/DataTable.h"
#include "Engine/World.h"
#include "Game/Save/SnSaveTypes.h"
#include "Game/SnSettings.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Straight down from the aim point. Deep enough to reach the floor under any
	 *  platform the player can aim at - UE's own ground-trace targeting uses the
	 *  same 99999. */
	constexpr float GroundTraceDepth = 99999.f;

	/** How far the down trace starts back along the aim direction. UE pulls back
	 *  "very slightly to avoid scraping down walls" - one unit along the aim
	 *  direction (GameplayAbilityTargetActor_GroundTrace.cpp:167) - because
	 *  starting the trace exactly on the face the aim ray just hit can begin
	 *  inside that collision. */
	constexpr float GroundTracePullBack = 1.f;

	/** Clips the camera ray to a sphere of Radius around Center and returns the
	 *  ray/sphere exit point - a port of UE's
	 *  AGameplayAbilityTargetActor_Trace::ClipCameraRayToAbilityRange
	 *  (GameplayAbilityTargetActor_Trace.cpp:134). It is what makes an aim's
	 *  reach a distance from the CHARACTER rather than from the camera: aiming
	 *  at the sky still yields a point at arm's length instead of one off the
	 *  map.
	 *
	 *  False when the ray points away from the sphere or misses it outright; the
	 *  caller then keeps its unclipped end point (UE does the same). */
	bool ClipCameraRayToRange(const FVector& CameraLocation, const FVector& CameraDirection, const FVector& Center, float Radius, FVector& OutClippedLocation)
	{
		const FVector CameraToCenter = Center - CameraLocation;
		const float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);
		if (DotToCenter < 0.f)
		{
			return false;
		}

		const float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		const float RadiusSquared = Radius * Radius;
		if (DistanceSquared > RadiusSquared)
		{
			return false;
		}

		// The far intersection (subtracting would give the near one, which for a
		// camera inside the sphere sits behind it).
		const float DistanceAlongRay = DotToCenter + FMath::Sqrt(RadiusSquared - DistanceSquared);
		OutClippedLocation = CameraLocation + (DistanceAlongRay * CameraDirection);
		return true;
	}
}

const FSnBuildDeviceRow* USnBuildLibrary::FindBuildDeviceRow(FName DeviceID)
{
	if (DeviceID.IsNone())
	{
		return nullptr;
	}

	const UDataTable* Table = USnSettings::Get()->GetBuildDeviceDataTable();
	if (!Table)
	{
		return nullptr;
	}
	return Table->FindRow<FSnBuildDeviceRow>(DeviceID, TEXT("USnBuildLibrary::FindBuildDeviceRow"));
}

FString USnBuildLibrary::GetBuildLevelName(const UWorld* World)
{
	if (!World)
	{
		return FString();
	}

	// UWorld::RemovePIEPrefix is the engine's own "UEDPIE_0_HubMap" -> "HubMap"
	// (World.cpp:9338) - the same normalization the save path applies to
	// FSnRunSave::LevelName, so a record and a saved level agree on what the
	// current level is called.
	return UWorld::RemovePIEPrefix(World->GetMapName());
}

bool USnBuildLibrary::ComputePlacementPoint(const APawn* Pawn, const APlayerController* PC, float PlacementRange, FVector& OutLocation, FRotator& OutRotation)
{
	OutLocation = FVector::ZeroVector;
	OutRotation = FRotator::ZeroRotator;

	UWorld* World = Pawn ? Pawn->GetWorld() : nullptr;
	if (!World || PlacementRange <= 0.f)
	{
		return false;
	}

	// Aim source: the camera's view point, NOT the pawn's facing - the pitch is
	// what decides how far ahead the device lands. A pawn with no player behind
	// it (AI, unpossessed) falls back to its own eye level and facing.
	FVector ViewLocation;
	FRotator ViewRotation;
	if (PC)
	{
		PC->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		ViewLocation = Pawn->GetPawnViewLocation();
		ViewRotation = Pawn->GetActorRotation();
	}
	const FVector ViewDirection = ViewRotation.Vector();

	const FVector Origin = Pawn->GetActorLocation();
	FVector ViewEnd = ViewLocation + (ViewDirection * PlacementRange);
	FVector ClippedEnd;
	if (ClipCameraRayToRange(ViewLocation, ViewDirection, Origin, PlacementRange, ClippedEnd))
	{
		ViewEnd = ClippedEnd;
	}

	// The pawn never blocks its own placement.
	FCollisionQueryParams Params(FName(TEXT("SnBuildPlacement")), /*bTraceComplex=*/false);
	Params.AddIgnoredActor(Pawn);

	// What is the player pointing at? The trace runs from the character to the
	// clipped camera-ray end, so a wall the camera itself sits inside still stops
	// it. Complex collision stays off on purpose: level geometry is what
	// placement cares about (UE's targeting traces are simple too), and the
	// range test rejects a hit that only exists because the clip failed and the
	// end point ended up outside the reach.
	FHitResult AimHit;
	World->LineTraceSingleByChannel(AimHit, Origin, ViewEnd, ECC_Visibility, Params);
	const bool bUseAimHit = AimHit.bBlockingHit
		&& FVector::DistSquared(Origin, AimHit.ImpactPoint) <= FMath::Square(PlacementRange);
	const FVector AimPoint = bUseAimHit ? AimHit.ImpactPoint : ViewEnd;

	// Straight down from the aim point for the ground the device stands on.
	const FVector AimDirection = (ViewEnd - Origin).GetSafeNormal();
	const FVector GroundTraceStart = AimPoint - (AimDirection * GroundTracePullBack);
	FHitResult GroundHit;
	World->LineTraceSingleByChannel(GroundHit, GroundTraceStart, GroundTraceStart - FVector(0.f, 0.f, GroundTraceDepth), ECC_WorldStatic, Params);
	if (!GroundHit.bBlockingHit)
	{
		// Aimed off the map: no floor below.
		return false;
	}

	OutLocation = GroundHit.ImpactPoint;
	// Yaw only - the device stands upright wherever the player looks, and no
	// caller has to defend against a stray pitch or roll.
	OutRotation = FRotator(0.f, ViewRotation.Yaw, 0.f);
	return true;
}

bool USnBuildLibrary::IsPlacementValid(const AActor* Avatar, const FVector& Location, float MinDistance)
{
	if (!Avatar)
	{
		return false;
	}

	// Horizontal distance on purpose: the landing spot is on the ground under
	// the crosshair, so standing on a ledge and looking straight down gives a
	// point directly below the character - a 3D distance would wave that through
	// on the height difference alone.
	return FVector::Dist2D(Location, Avatar->GetActorLocation()) >= MinDistance;
}
