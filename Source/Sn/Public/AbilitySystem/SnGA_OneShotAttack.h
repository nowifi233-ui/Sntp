// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_SphereAttack.h"
#include "SnGA_OneShotAttack.generated.h"

/**
 * One-shot (use-item) attack ability: the shared sphere-attack core
 * (USnGA_SphereAttack) under the transient, server-initiated lifecycle of
 * the UseAbility item path. A use from the bag or an item action grants it
 * through the engine one-shot API (GiveAbilityAndActivateOnce - see
 * ServerUseItem): the spec is removed the moment the instance ends, and the
 * engine clears it itself on activation failure. A use from the hotbar takes
 * the held window instead: the spec is granted while the item is the selected
 * slot and each press only activates it. Either way one use = one instance,
 * and nothing in here may assume the spec dies with the use - that is exactly
 * what InstancedPerExecution buys (a lingering spec holds no instance, so the
 * next press activates cleanly).
 *
 * ServerOnly on purpose. The engine one-shot activation carries an empty
 * prediction key, which cannot echo a LocalPredicted activation of a remote
 * player's ability (no local activation exists for the key to absorb); and a
 * same-frame ability never survives long enough for the ServerInitiated
 * activation push (ClientActivateAbilitySucceed) to reach the owning client
 * - its grant and removal collapse into one replicated update, so the client
 * never sees the spec and the activation parks in
 * PendingServerActivatedAbilities forever. ServerOnly runs the grant ->
 * activate -> remove lifecycle cleanly in one server frame. The instance
 * therefore only ever runs on the server: damage / knockback / hit stop /
 * kill credit are authoritative, and the swing + hit cues the core executes
 * multicast to every side (an empty prediction key means no client predicted
 * this activation, so nobody absorbs the echo - the notify assets play once
 * per machine). The attacker's own copy arrives with the multicast echo, one
 * round trip later than a predicted attack - the accepted cost of the
 * transient grant, invisible on LAN.
 *
 * One element per skill (single-element model, same as the input attack):
 * DamageType / Multiplier / DamageEffectClass / cue tags are picked on the
 * blueprint defaults - a fire skill is a blueprint with DamageType = Fire.
 */
UCLASS()
class SN_API USnGA_OneShotAttack : public USnGA_SphereAttack
{
	GENERATED_BODY()

public:
	USnGA_OneShotAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};
