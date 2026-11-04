// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_DamageSkill.h"
// EDamageType lives here - the header's members need the full type, not a
// forward declaration (same include as the aura/enchantment ability headers).
#include "Combat/USnCombatLibrary.h"
// FGameplayTag (SwingCueTag / HitCueTag below).
#include "GameplayTagContainer.h"
#include "SnGA_SphereAttack.generated.h"

class UAnimMontage;
class UGameplayEffect;

/**
 * Shared forward-sphere attack core: damages every ASC-capable enemy in a
 * sphere in front of the attacker, then knocks each target back away from
 * the attacker. One element per hit: damage = the attacker's damage of
 * DamageType (base x (1 + percent)) times Multiplier, growing per ability
 * level (PerLevelGrowth; 0 = level-independent).
 *
 * Not an ability by itself - the three attack roles subclass this and supply
 * their own ActivateAbility: the input attack (USnGA_Attack: permanent
 * grant, LocalPredicted), the one-shot use-item attack (USnGA_OneShotAttack:
 * granted per use via GiveAbilityAndActivateOnce, ServerOnly) and the enemy
 * attack (USnGA_EnemyAttack: granted to the enemy's own ASC at init and
 * activated by its AI controller, ServerOnly). The roles differ only in
 * lifecycle and net policy - ALL
 * presentation is gameplay cues fired here by ExecuteSphereAttack: the
 * swing cue (SwingCueTag, once per activation, plays on a miss too, its
 * RawMagnitude carries the hit flag for the camera shake) and the per-target
 * hit cue (HitCueTag at every damaged target). The predicted input attack
 * executes its cues on the owning client with a prediction key (instant
 * feel); the server's echo carries the same key back, and keys only
 * replicate to their originator - the receiving side skips the cues it
 * predicted (GameplayPrediction.h "Gameplay Cue Events"), so every machine
 * plays exactly one copy. The one-shot has no prediction (empty key): its
 * cues run once on the server and multicast to all sides.
 *
 * The montage-driven hit frame is shared too (PlayAttackMontageAndWaitForWindow):
 * a role with a swing animation does not resolve damage at activation but at
 * the animation's own notify, and the roles would otherwise each wire the same
 * two tasks. The montage asset and its play settings stay fields of the
 * calling role class (per-role CDO config - see USnGA_DamageSkill's field
 * policy), so the helper takes them as arguments.
 *
 * Whom the body DAMAGES is the other role-level difference
 * (IsValidAttackTarget): the default is the enemy team tag every player-side
 * skill uses; USnGA_EnemyAttack reverses it. The combat library's hostility
 * source stays untouched, so no player-facing query changes behaviour.
 *
 * As a family template, this class is also the project's skill-template
 * contract (Docs/SKILL_TEMPLATE.md):
 * - A new skill of this family = assets only, no C++: a shell blueprint
 *   derived from a role class, its numbers configured in Class Defaults
 *   (element, Multiplier, sphere shape, knockback, Swing/Hit cue tags),
 *   plus the pure-data GC assets the tags point at (GC_Swing_<skill>
 *   deriving from USnGCN_Swing, GC_Hit_<skill> from the engine's
 *   UGameplayCueNotify_Burst).
 * - A new role (grant + net policy) = one thin subclass supplying its own
 *   ActivateAbility around the shared body.
 * - A new damage-detection shape (projectile, sector, self-buff, dash) is a
 *   NEW family: a new core class modeled on this one, with its own roles.
 *   The projectile family (USnGA_ProjectileAttack + ASnProjectile) already
 *   lives beside this one, under the shared damage base (USnGA_DamageSkill -
 *   function-level only: the damage formula + description clause; fields
 *   stay duplicated per family, see its class doc). The timed-trigger
 *   lifecycle family (USnGA_TimedTrigger: sustain / volley shells releasing
 *   configured child GAs) and the summon window family (USnGA_Summon)
 *   complete the template set.
 */
UCLASS()
class SN_API USnGA_SphereAttack : public USnGA_DamageSkill
{
	GENERATED_BODY()

public:
	/** Damage effect applied to every target hit (e.g. GE_Damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Element of the attack's damage (single-element model: one attack deals
	 *  exactly one element; extra damage is a second, independent ability). */
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
	 *  level-independent (the basic attack). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (ClampMin = "0.0"))
	float PerLevelGrowth = 0.f;

	/** Swing cue tag (GameplayCue.Swing.<skill>): fired once per activation,
	 *  hit or miss, at the end of the shared body. The notify asset
	 *  (GC_Swing_<skill>) spawns at the cue's Location (avatar + 50 cm
	 *  forward) facing its Normal, plays the arc VFX + whoosh, and - when
	 *  RawMagnitude = 1 (something took damage) - the once-per-swing camera
	 *  shake, only for the locally controlled attacker. Invalid tag = no
	 *  swing cue. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag SwingCueTag;

	/** How far in front of the attacker the damage sphere's center sits. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float SphereForwardDistance = 150.f;

	/** Radius of the damage sphere. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float SphereRadius = 200.f;

	/** Horizontal knockback strength (direction: attacker -> target). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float LaunchForce = 800.f;

	/** Vertical knockback added on top of LaunchForce. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float LaunchUpForce = 200.f;

	/** Hit cue tag (GameplayCue.Hit.<skill>): fired at every target that took
	 *  damage, inside the shared body's per-target loop. The notify asset
	 *  (GC_Hit_<skill>) spawns its impact spark / sound at the cue's Location
	 *  (the target's position). Invalid tag = no hit cue. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FGameplayTag HitCueTag;

	/** Hit stop: every damaged target gets CustomTimeDilation set to this
	 *  value for HitStopDuration. 0 (the default) disables hit stop -
	 *  ApplyHitStop early-outs, so skills opt in per asset. The restore
	 *  timer binds the target actor itself (weak lambda), so the restore
	 *  always runs while the target lives and dies with it otherwise - never
	 *  bind the restore to this GA: blueprint abilities are instanced per
	 *  execution, the instance is destroyed at EndAbility and a pending
	 *  timer on it silently vanishes, leaving the target frozen forever.
	 *  World timers are NOT scaled by CustomTimeDilation, so the stop is
	 *  exact. When enabled keep it small but above ~0.03 - near-zero
	 *  dilations leave the mesh's animation pose with interpolation
	 *  artifacts. Only targets freeze, never the attacker (its camera
	 *  spring arm lives on the pawn - freezing the pawn would freeze the
	 *  camera). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float HitStopTimeDilation = 0.f;

	/** How long hit-stopped targets stay frozen before their dilation is restored. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float HitStopDuration = 0.08f;

	/** Description: final damage percentage at Level (Multiplier +
	 *  Level x PerLevelGrowth, rounded) of the configured element's damage,
	 *  rendered with the element's rich-text style row. */
	virtual FText GetDescription_Implementation(int32 Level) override;

protected:
	/** Target filter of the shared body: an actor takes damage when this
	 *  returns true. Default = USnCombatLibrary::IsEnemy (the Combat.Team.Enemy
	 *  tag) - the player-facing rule, unchanged for every existing skill.
	 *  USnGA_EnemyAttack overrides it with the reversed check (the player team
	 *  tag), so one core serves both directions without the combat library's
	 *  IsEnemy losing its meaning (it stays "is a valid target FOR THE PLAYER"
	 *  everywhere else: projectiles, FindNearestEnemy, companion homing).
	 *  Destructibles never reach this - the core branches on them first. */
	virtual bool IsValidAttackTarget(AActor* Target) const;

	/** Whether this role also turns the avatar's body onto the aim when the
	 *  strike resolves. Default false: the aim is only ever READ
	 *  (USnGA_DamageSkill::GetAvatarAimRotation), so activating a skill never
	 *  rotates the character - a body snap turns the whole rig and, because the
	 *  camera boom re-sets its world rotation from the control rotation every
	 *  tick, drags the camera for that frame (the jitter the projectile skills
	 *  showed). USnGA_Attack - the input basic attack - is the one role that
	 *  overrides this: its swing animation and held weapon should read as
	 *  hitting what the player aimed at. Purely visual either way: the sphere,
	 *  the cues and the knockback all read the aim yaw, never the body's facing. */
	virtual bool ShouldFaceAimOnStrike() const { return false; }

	/** Plays InMontageToPlay and arms the shared hit-frame lifecycle: the damage
	 *  body resolves per Event.AttackWindow notify (a multi-notify montage hits
	 *  several times in one swing), and the ability ends when the montage does.
	 *  Both montage roles wire exactly these two tasks and these delegates -
	 *  this owns that wiring so they do not each repeat it. The montage itself
	 *  and its play settings stay fields of the calling role (see the class
	 *  doc); a null montage is a no-op, so the caller keeps its no-montage
	 *  branch. */
	void PlayAttackMontageAndWaitForWindow(UAnimMontage* InMontageToPlay, float InPlayRate, FName InStartSection);

	/** The shared gameplay body - damage calculation, sphere overlap, the
	 *  per-target damage / hit event / hit cue / knockback / hit stop loop,
	 *  and the swing cue at the end. All presentation runs from here as
	 *  gameplay cues (see SwingCueTag / HitCueTag), so the roles only supply
	 *  the lifecycle around it. Callers must guard SourceASC / Avatar /
	 *  DamageEffectClass first (this never ends the ability). */
	void ExecuteSphereAttack(UAbilitySystemComponent* SourceASC, AActor* Avatar);

	/** Applies CooldownGameplayEffectClass to the attacker. Still a plain GAS
	 *  cooldown effect (the engine's CanActivateAbility gate keeps rejecting
	 *  activations while the granted Cooldown.Attack tag is up) but applied
	 *  manually rather than through CommitAbility, so the duration can scale
	 *  with the attacker's AttackSpeed (CD / (1 + AttackSpeed)). No-op when
	 *  the class is unset. Callers must guard SourceASC / Avatar first.
	 *
	 *  The roles call this at their OWN activation frame, not from the damage
	 *  body: USnGA_Attack resolves its damage at the montage's hit frame, so a
	 *  cooldown started there would hand a free re-swing to anyone cancelled
	 *  or killed mid wind-up. */
	void ApplyAttackCooldown(UAbilitySystemComponent* SourceASC, AActor* Avatar);

private:
	/** Montage hit frame (Event.AttackWindow): runs the shared damage body.
	 *  Fires per notify, so a montage with several notifies resolves several
	 *  times within one swing. Authority-gated - see the class doc. */
	UFUNCTION()
	void OnAttackWindowEvent(FGameplayEventData Payload);

	/** The montage is over (completed / blended out / interrupted /
	 *  cancelled): ends the ability, releasing it for the next swing. Bound to
	 *  all four pins on purpose - see PlayAttackMontageAndWaitForWindow. */
	UFUNCTION()
	void OnAttackMontageFinished();

	/** Freezes Target and arms its own restore timer (see HitStopTimeDilation). */
	void ApplyHitStop(AActor* Target);
};
