// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_TimedTrigger.h"
#include "SnGA_SustainTrigger.generated.h"

/**
 * Sustain (periodic-release) trigger shell: the timed-trigger core under the
 * equip-window lifecycle. Releases TriggeredAbilityClass every
 * IntervalSeconds for as long as the ability stays active; infinite by
 * default (MaxTriggerCount 0 - the asset keeps releasing until the ability is
 * ended from outside: unequip, cancel, level travel). The first release fires
 * on activation (InitialDelay 0).
 *
 * The sustain window maps onto the equipment window (the summon ability
 * model): the equip flow grants AND activates this ability in one step
 * (bActivateOnEquip, read by USnItemLibrary::GrantEquipAbilities) - equipping
 * starts the rhythm, the ability stays active for the whole equip duration,
 * and EndAbility revokes the child grant. No OnAvatarSet self-activation: the
 * equip flow owns activation (see the bActivateOnEquip doc on the base
 * class). A sustain releasing a summon ability is the summon-stream combo -
 * the child only re-releases once its previous summon is gone (see the
 * USnGA_TimedTrigger class doc).
 *
 * Asset config: TriggeredAbilityClass = the released GA, IntervalSeconds /
 * InitialDelay / MaxTriggerCount = rhythm, engine cooldown fields optional
 * (gates whole runs, see the core doc).
 */
UCLASS()
class SN_API USnGA_SustainTrigger : public USnGA_TimedTrigger
{
	GENERATED_BODY()

public:
	USnGA_SustainTrigger();
};
