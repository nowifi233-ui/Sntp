// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "USnBuildLibrary.generated.h"

class APawn;
class APlayerController;
class UWorld;
struct FSnBuildDeviceRow;

/**
 * Placement rules for player building: where a device would land and whether
 * that spot is allowed. Pure functions over (pawn, aim, world) - no ability
 * state, no owner, no side effects - so the one caller today (USnGA_Build)
 * and a future client-side preview share one implementation of "where does it
 * go" instead of drifting apart into a ghost that lies about the result.
 *
 * The geometry follows UE's own targeting actors rather than a fresh
 * invention: the camera ray is clipped to a sphere around the CHARACTER
 * (AGameplayAbilityTargetActor_Trace::ClipCameraRayToAbilityRange), then a
 * ground trace runs straight down from the aim point
 * (AGameplayAbilityTargetActor_GroundTrace::PerformTrace).
 */
UCLASS()
class SN_API USnBuildLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** The placement geometry used when nothing overrides it, so "where does a
	 *  device land" has one answer outside the ability too. USnGA_Build seeds its
	 *  EditDefaultsOnly fields from these (a blueprint may still retune either
	 *  number per device), and the PlaceDevice cheat - which has no ability
	 *  instance to read - uses them directly. Two callers, one pair of numbers. */
	static constexpr float DefaultPlacementRange = 400.f;
	static constexpr float DefaultMinDistance = 150.f;

	/** Row lookup in the global build-device table (USnSettings), or nullptr. */
	static const FSnBuildDeviceRow* FindBuildDeviceRow(FName DeviceID);

	/** The level key a build record is filed under: the map name with its PIE
	 *  prefix stripped ("UEDPIE_0_HubMap" -> "HubMap"). Deliberately the same
	 *  normalization FSnRunSave::LevelName gets from the save path, so a record
	 *  and a saved level agree on what the current level is called. */
	static FString GetBuildLevelName(const UWorld* World);

	/** Where a device aimed from the camera would land, or false when there is no
	 *  ground under the aim point (aiming off the map).
	 *
	 *  PC may be null (AI-led or unpossessed pawn): the aim then comes from the
	 *  pawn's own eye level and facing, so the function stays total for callers
	 *  that have no player behind them.
	 *
	 *  OutRotation is always yaw-only - a device stands upright no matter where
	 *  the player is looking, and nothing downstream has to defend against a
	 *  stray pitch/roll. */
	static bool ComputePlacementPoint(const APawn* Pawn, const APlayerController* PC, float PlacementRange, FVector& OutLocation, FRotator& OutRotation);

	/** Whether a computed landing spot passes the placement rules. Kept apart
	 *  from ComputePlacementPoint so "where it would go" and "is that allowed"
	 *  stay separately answerable (and separately testable).
	 *
	 *  v1: far enough from the character. The space check (does a wall / a
	 *  stored device / an enemy already occupy the spot) belongs here too once
	 *  the preview lands - it changes what the ghost shows, not the aim. */
	static bool IsPlacementValid(const AActor* Avatar, const FVector& Location, float MinDistance);
};
