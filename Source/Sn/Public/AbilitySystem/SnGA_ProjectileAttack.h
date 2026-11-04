// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_DamageSkill.h"
// The projectile class is needed in full - UHT only emits a pickable class
// pin for TSubclassOf<T> when T is defined (see the same note in
// USnCombatLibrary.h).
#include "Combat/ASnProjectile.h"
// EDamageType lives here (DamageType / BuildDamageClause).
#include "Combat/USnCombatLibrary.h"
// FGameplayTag (SwingCueTag / HitCueTag below).
#include "GameplayTagContainer.h"
#include "SnGA_ProjectileAttack.generated.h"

class UGameplayEffect;

/**
 * Projectile attack family core: fires ONE projectile ahead of the attacker
 * and ends - the launch is a single-frame body, everything after it lives on
 * the spawned projectile (ASnProjectile: engine ProjectileMovement flight,
 * server-side hit resolution, impact cue, wall/lifetime cleanup). Damage is
 * pre-built into a spec at launch (USnCombatLibrary::MakeDamageSpec - the
 * delayed-delivery API: same spec ApplyDamageToTarget would apply instantly,
 * carried by the projectile and applied on hit), so the numbers are decided
 * at launch time and the flight is pure presentation until impact.
 *
 * As a family template this class is a shell AND the core in one (no derived
 * role classes yet): ServerOnly + InstancedPerExecution, same transient
 * single-frame shape as USnGA_OneShotAttack, so every existing trigger entry
 * takes it directly:
 * - use-item one-shot: item row UseAbility -> GiveAbilityAndActivateOnce
 *   (AbilityTags must NOT carry Ability.Attack - same rule as the one-shot)
 * - timed-trigger child: a SustainTrigger / VolleyTrigger shell pre-grants
 *   this class and releases it per pulse (single-frame body satisfies the
 *   child constraint; pulse rhythm belongs to the shell, no per-pulse CD)
 * - bActivateOnEquip: an equip row that wants a standing "one shot ready"
 *   passive also works (grant + activate in one, equip-flow owned)
 * Damage / hit resolution / kill credit are authoritative; presentation is
 * gameplay cues from the server (empty prediction key - this family is NOT
 * predicted: community consensus is to leave projectile damage unpredicted,
 * and the launch itself is server-initiated from every entry above, so no
 * local copy exists for a prediction key to echo. A ranged basic-attack feel
 * (input shell + client fakey) is the documented upgrade path, see
 * Docs/GA_PROJECTILE.md §5).
 *
 * Homing is parameterized on this one class, not a separate family: bHoming
 * makes the launcher search FindNearestEnemy within HomingSearchRadius of
 * the muzzle and aim the first shot at it; the projectile then tracks it
 * (HomingTargetComponent set at runtime - engine rule), and a target that
 * dies mid-flight leaves the missile flying straight (engine-native). The
 * missile's tracking strength (HomingAccelerationMagnitude) is physics and
 * lives on the projectile blueprint - the GA only decides whether to hunt.
 *
 * Numbers / presentation split (one-place rule): element / Multiplier /
 * PerLevelGrowth / damage GE / MuzzleOffset / homing search live in the
 * blueprint Class Defaults; the flight physics + visuals (speed, gravity,
 * turn rate, mesh, lifetime) live on the projectile blueprint CDO.
 * Description generated for the UI from the shared damage-family clause.
 */
UCLASS()
class SN_API USnGA_ProjectileAttack : public USnGA_DamageSkill
{
	GENERATED_BODY()

public:
	USnGA_ProjectileAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Description: the shared element clause in the family sentence (bHoming
	 *  skills announce the tracking). Same formula the launcher used for the
	 *  pre-built spec, so the panel matches the damage. */
	virtual FText GetDescription_Implementation(int32 Level) override;

	/** Damage effect applied to the hit target (e.g. GE_Damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Element of the attack's damage (single-element model, same as the
	 *  sphere family: one attack deals exactly one element). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	EDamageType DamageType = EDamageType::Physical;

	/** Damage = the attacker's damage of DamageType (base x (1 + percent))
	 *  times this multiplier (1 = the element damage as-is, 0.5 = half). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float Multiplier = 1.f;

	/** Extra multiplier added per ability level - a FLAT addition to
	 *  Multiplier, not a compound factor: effective multiplier = Multiplier +
	 *  Level x PerLevelGrowth (0.1 = +0.1 to the multiplier per level,
	 *  whatever Multiplier is; level 0 - the starting level - applies the
	 *  multiplier as-is, skills and equipment levels count from 0). 0 =
	 *  level-independent. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float PerLevelGrowth = 0.f;

	/** Projectile spawned at the muzzle (an ASnProjectile blueprint; its CDO
	 *  owns the flight physics, collision and visuals - see ASnProjectile). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<ASnProjectile> ProjectileClass;

	/** Launch cue tag (GameplayCue.Swing.<skill>): fired once per activation
	 *  at the muzzle when the projectile leaves. The notify asset
	 *  (GC_Swing_<skill>) spawns at the cue's Location facing its Normal.
	 *  RawMagnitude = 0 (nothing took damage at launch), so the notify's
	 *  once-per-swing camera-shake gate stays off - the impact cue carries
	 *  the hit. Invalid tag = no launch cue. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag SwingCueTag;

	/** Hit cue tag (GameplayCue.Hit.<skill>): fired by the projectile at the
	 *  impact point when it damages an enemy (passed to it at spawn). Invalid
	 *  tag = no hit cue. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag HitCueTag;

	/** How far in front of the attacker the projectile spawns. (Upgrade path
	 *  if a hand/socket launch ever matters: turn this into a socket name.) */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float MuzzleOffset = 150.f;

	/** Homing: when true, the launcher seeks the nearest enemy within
	 *  HomingSearchRadius of the muzzle and aims the first shot at it (the
	 *  projectile then tracks it - the target's death leaves it flying
	 *  straight). No target in range = a normal forward shot, never a fail. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	bool bHoming = false;

	/** How far out the homing launch looks for a target (see bHoming). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0", EditCondition = "bHoming"))
	float HomingSearchRadius = 1000.f;
};
