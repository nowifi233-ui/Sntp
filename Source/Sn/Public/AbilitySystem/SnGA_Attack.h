// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_SphereAttack.h"
#include "SnGA_Attack.generated.h"

class UAnimMontage;

/**
 * Input attack ability (the basic attack): the shared sphere-attack core
 * (USnGA_SphereAttack) under the permanent-grant, client-predicted
 * lifecycle. Activated by the input path (TryActivateAbilitiesByTag on
 * Ability.Attack) on every swing.
 *
 * With a montage configured (the shipped setup) activation starts the
 * cooldown, then plays MontageToPlay - on the owning client's predicted copy
 * and on the server's authoritative copy - and waits for the montage's
 * SnAnimNotify_GameplayEvent (Event.AttackWindow) to fire the damage body at
 * the animation's hit frame: the swing lands when the weapon does, not when
 * the button went down. The wiring and the authority rule that go with it are
 * the sphere core's (PlayAttackMontageAndWaitForWindow / the class doc): this
 * role supplies the montage asset and its play settings, the shared helper
 * runs the same lifecycle for every montage role.
 *
 * Without a montage the ability keeps the original activate-frame behaviour:
 * the body runs inside ActivateAbility on both predicted copies and the shared
 * prediction key dedups their cues (see USnGA_SphereAttack).
 *
 * Instanced per actor on purpose: the ability is permanently granted, and the
 * engine refuses to re-activate an active InstancedPerActor ability, so a
 * held-down or spammed button cannot start a second swing on top of the
 * running montage (two instances would fight over the anim slot and land the
 * damage twice). The ability lives for the length of the montage, so the
 * swing also blocks itself until the animation is done.
 *
 * The one role that also turns the body onto the aim when the strike resolves
 * (ShouldFaceAimOnStrike): the swing animation and the held weapon should read
 * as hitting what the player aimed at. Every other role - the item one-shots,
 * the projectile family, the enemy swings - reads the aim without touching the
 * facing (USnGA_DamageSkill::GetAvatarAimRotation); a body snap turns the whole
 * rig and drags the camera for that frame.
 *
 * One element per skill (single-element model): DamageType / Multiplier /
 * DamageEffectClass / cue tags are picked on the blueprint defaults - the
 * defaults (Physical / 1.0 / no growth) reproduce the basic attack.
 * Use-item attack skills are their own class (USnGA_OneShotAttack): a
 * transient per-use grant must not share this class's Ability.Attack tag or
 * LocalPredicted lifecycle - a leaked one-shot spec in the ASC would re-fire
 * on the next TryActivateAbilitiesByTag.
 *
 * Caveat: the hit window is driven by the montage's own notifies, which need a
 * real AnimInstance. A headless dedicated server would never reach the window
 * and the swing would deal no damage; LAN listen-server play is unaffected
 * (the host runs a full client and plays the montage like anyone else).
 */
UCLASS()
class SN_API USnGA_Attack : public USnGA_SphereAttack
{
	GENERATED_BODY()

public:
	USnGA_Attack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Swing montage, played on activation. Put a Sn Gameplay Event notify on
	 *  its hit frame with Event.AttackWindow (one per damage frame - a
	 *  multi-notify montage is a multi-hit combo). Empty = no montage: the
	 *  damage resolves at activation instead (the pre-montage behaviour). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Montage")
	TObjectPtr<UAnimMontage> MontageToPlay;

	/** Montage play rate (1 = the asset's own speed). Attack speed does not
	 *  feed this - it only shortens the cooldown. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Montage", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.f;

	/** Montage section to start from (None = from the start). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Montage")
	FName MontageStartSection = NAME_None;

protected:
	/** See the class doc: the basic attack is the one role that turns the body
	 *  onto the aim when the swing resolves. */
	virtual bool ShouldFaceAimOnStrike() const override { return true; }
};
