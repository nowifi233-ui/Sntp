// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_TimedTrigger.h"
#include "SnGA_VolleyTrigger.generated.h"

/**
 * Volley (release-and-finish) trigger shell: the timed-trigger core under the
 * one-shot item lifecycle. Releases TriggeredAbilityClass a fixed
 * MaxTriggerCount times (default 3, quick succession) and ends itself when
 * the count is met - the use-item path (item row UseAbility) is the intended
 * entry, so one volley = one item use. On the hotbar that entry is the held
 * spec (granted while the item is the selected slot, activated per press); a
 * bag or item-action use grants it through the engine one-shot API instead.
 * Both press the same button, and the count is per activation either way.
 *
 * Asset config: MaxTriggerCount = releases per use, IntervalSeconds = rhythm
 * between releases (0 = zero-gap burst), TriggeredAbilityClass = the released
 * GA (a single-frame one-shot attack body - every release must run to
 * completion within its own frame or the next one gets swallowed).
 * AbilityTags must NOT carry Ability.Attack - the input sweep would pick the
 * volley's grant up as a basic attack. bActivateOnEquip stays false (the
 * equip auto-activation opt-in): volleys are only ever activated on demand
 * from the use-item path.
 */
UCLASS()
class SN_API USnGA_VolleyTrigger : public USnGA_TimedTrigger
{
	GENERATED_BODY()

public:
	USnGA_VolleyTrigger();
};
