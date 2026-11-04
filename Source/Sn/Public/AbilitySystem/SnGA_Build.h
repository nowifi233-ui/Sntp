// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGameplayAbility.h"
#include "Game/USnBuildLibrary.h"
#include "SnGA_Build.generated.h"

/**
 * Places one device from the build-device table (USnSettings) where the player
 * is looking - the item-driven half of building. The item row's UseAbility
 * points at a blueprint of this class (one per device, e.g.
 * BP_GA_Build_Turret with DeviceID=Turret), so "consume the kit, put the
 * turret down" is a data row plus a blueprint, no new code per device.
 *
 * ServerOnly + InstancedPerExecution, the same contract as the one-shot
 * attack: nothing here may wait for a client. The two ways in are the two ways
 * an item gets used - a hotbar use activates the spec the selected slot already
 * granted (USnInventoryComponent::RefreshHotbarGrant: the kit is "held" while
 * it is the slot in hand), while a use from the bag or an item action has no
 * window to hang on and goes through the engine's one-shot API
 * (GiveAbilityAndActivateOnce - see ServerUseItem): empty prediction key,
 * grant + activate + remove inside one server frame.
 *
 * The failure contract is what makes an invalid spot free, and the whole
 * placement check therefore lives in CanActivateAbility. Either way the use
 * path returns before the cooldown starts or the item is consumed - but only
 * by different means: the one-shot path gets an INVALID handle back because the
 * engine clears the spec when activation fails
 * (AbilitySystemComponent_Abilities.cpp:366-372), while the held path gets
 * false from TryActivateAbility and keeps its spec, so the player can step to a
 * better spot and press again. No rollback code - an impossible placement
 * simply never happened. CommitAbility is deliberately never called: the item's
 * own UseCD is the only gate, and a second one per device would make the kit's
 * cooldown depend on which row it came from.
 *
 * A blueprint child needs no logic at all: placement feedback (sound, dust,
 * the build animation) belongs on the device blueprint, which the server
 * spawns and every client sees appear.
 */
UCLASS()
class SN_API USnGA_Build : public USnGameplayAbility
{
	GENERATED_BODY()

public:
	USnGA_Build();

	/** Which device this ability places: the row name the placement ghost looks
	 *  up. Read off the class default object - a spec holds the CDO, not an
	 *  instance (InstancedPerExecution), so the ghost, the item row and the
	 *  activation itself all read these same values. */
	FName GetDeviceID() const { return DeviceID; }

	/** The ability's own placement geometry - the ghost computes its position
	 *  with these, not with the library defaults, so a blueprint that retunes
	 *  one device's reach retunes its ghost with it. */
	float GetPlacementRange() const { return PlacementRange; }
	float GetMinDistance() const { return MinDistance; }

	/** Full placement check (row exists, ground found, spot allowed). Runs on
	 *  the CDO - see the implementation for what that forbids. False here means
	 *  "the item is not consumed and no cooldown starts". */
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Tooltip line for the item that places this device: the device's own name,
	 *  taken from the build-device row, in one sentence of game copy. Composed
	 *  from the row rather than typed per blueprint, so renaming a device can
	 *  never leave an item describing a turret the table no longer has. The item
	 *  side renders it as "<ability name>：<this text>" - see
	 *  USnItemLibrary::GetUseAbilityDescriptionLines. */
	virtual FText GetDescription_Implementation(int32 Level) override;

protected:
	/** Row name (ID) in the build-device data table (USnSettings): which device
	 *  this ability places. One blueprint per device. */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	FName DeviceID;

	/** How far the aim reaches, as a sphere around the CHARACTER rather than the
	 *  camera: aiming at the sky still lands at this distance instead of
	 *  nowhere. Defaults to the shared value so the PlaceDevice cheat places
	 *  where the item would; a blueprint may retune it per device. */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	float PlacementRange = USnBuildLibrary::DefaultPlacementRange;

	/** Closest allowed landing distance from the character (horizontal, to match
	 *  the aim point being on the ground) - keeps a device from landing on the
	 *  player's own feet when the camera looks straight down. */
	UPROPERTY(EditDefaultsOnly, Category = "Build")
	float MinDistance = USnBuildLibrary::DefaultMinDistance;
};
