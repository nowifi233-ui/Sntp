// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_SphereAttack.h"
#include "SnGA_EnemyAttack.generated.h"

class UAnimMontage;

/**
 * The enemy's swing: the shared sphere-attack core under the enemy's own role
 * (the third of the three attack roles) - granted permanently to the enemy's
 * own ASC at init (ASnEnemy::InitAbilityActorInfo) and activated by the enemy
 * itself while its chase target stands inside ASnEnemy::AttackRange
 * (ASnEnemy::Tick -> TryActivateAbility), ServerOnly.
 *
 * Reversed hostility: the body damages whoever carries the player team tag
 * (IsValidAttackTarget override) - the mirror of the player-facing rule, so
 * one core serves both directions while USnCombatLibrary::IsEnemy keeps
 * meaning "a valid target for the player" for the projectiles and the homing.
 *
 * ServerOnly because the AI that activates it lives on the server only and the
 * swing's numbers are authoritative; clients never run an instance. They still
 * see the swing: the ability system replicates the montage itself
 * (PlayMontageInternal -> RepAnimMontageInfo -> OnRep_ReplicatedAnimMontage,
 * which for an AI pawn is never locally controlled and so always replays), and
 * the core's Swing/Hit cues multicast with an empty prediction key - exactly
 * once per machine, since there is no local activation to absorb them.
 *
 * The pace is the ability's own cooldown, not a timer (GE_EnemyAttackCooldown
 * grants Cooldown.EnemyAttack, CooldownTags lists it): while the tag is up the
 * engine's CanActivateAbility refuses the activation, so the enemy's per-frame
 * retry is simply silent until the next swing is ready - the project's
 * "cooldowns are GAS-native" rule. CooldownTags is required, not decorative:
 * an empty container short-circuits the engine's cooldown check.
 *
 * Instanced per actor on purpose: one instance per enemy, and an ACTIVE
 * instance is the "mid-swing" state the AI reads (ASnEnemy::IsAttacking) to
 * hold still until the animation is done. The engine also refuses to
 * re-activate an active InstancedPerActor ability, so the per-frame retry
 * cannot start a second swing on top of the running montage.
 *
 * Configure per enemy in the blueprint defaults: DamageEffectClass (the
 * touch-damage GE is the natural starting point), AttackMontage with an
 * SnAnimNotify_GameplayEvent (Event.AttackWindow) on its hit frame, the
 * sphere shape and the knockback. A contact-range swing usually wants a
 * smaller sphere and a weaker knockback than the player's (the old touch
 * damage's feel was 250 / 80), and HitStopTimeDilation should stay 0 - a
 * hit-stopped player is a hard stun lock. AbilityTags must NOT contain
 * Ability.Attack: that tag is the player's input path (its cooldown ring
 * watches it), and the AI activates this ability by handle anyway.
 */
UCLASS()
class SN_API USnGA_EnemyAttack : public USnGA_SphereAttack
{
	GENERATED_BODY()

public:
	USnGA_EnemyAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Swing montage, played on activation. Put an Sn Gameplay Event notify on
	 *  its hit frame with Event.AttackWindow (one per damage frame). Empty = no
	 *  montage: the damage resolves at activation instead - a misconfigured
	 *  enemy still fights, it just loses the wind-up. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Montage")
	TObjectPtr<UAnimMontage> AttackMontage;

	/** Montage play rate (1 = the asset's own speed). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Montage", meta = (ClampMin = "0.01"))
	float MontagePlayRate = 1.f;

	/** Montage section to start from (None = from the start). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack|Montage")
	FName MontageStartSection = NAME_None;

protected:
	/** Reversed: the enemy swing damages the PLAYER team, not the enemy team.
	 *  See the class doc - the combat library's IsEnemy is not involved. */
	virtual bool IsValidAttackTarget(AActor* Target) const override;
};
