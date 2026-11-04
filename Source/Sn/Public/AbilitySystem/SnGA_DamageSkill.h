// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGameplayAbility.h"
// EDamageType lives here - the helper signatures need the full type.
#include "Combat/USnCombatLibrary.h"
#include "SnGA_DamageSkill.generated.h"

/**
 * Shared function-level base of the damage families: the damage shapes (the
 * sphere attack and the projectile attack) apply the same level-scaled
 * formula and describe it with the same rich-text clause as the two users
 * with no shape of their own - the event-triggered enchantment / ignite
 * passives (USnGA_Enchantment / USnGA_Ignite, hit target comes from the
 * event) and the event-proc family's damage payload (USnGA_EventProc, whose
 * damage is a share of the triggering amount). Holds ONLY the code they truly
 * repeat - the level-scaled damage formula and the rich-text element clause
 * of the descriptions. Config fields deliberately stay DUPLICATED on each
 * family class (Multiplier / PerLevelGrowth / DamageType / DamageEffectClass /
 * cue tags, each declared on its own UCLASS): every existing blueprint hangs
 * off USnGA_Attack / USnGA_OneShotAttack / USnGA_SphereAttack by class name,
 * so reparenting the C++ chain is a no-op for assets, while moving properties
 * up the chain would touch their serialized CDO paths for zero gain. Move the
 * fields up only when a family arrives that would actually duplicate them -
 * the shape-free users borrow these FUNCTIONS and carry no damage fields of
 * their own (USnGA_EventProc's Damage* fields are its payload's own config,
 * not a copy of the sphere family's).
 *
 * Families that repeat more than these two segments may add their own helpers
 * here (cooldown application is still sphere-family-only - projectile skills
 * take their cooldown from the item-use gate or a trigger shell, so it stays
 * there until a real duplicate appears). Abstract: never a standalone
 * ability, only a parent.
 */
UCLASS(Abstract)
class SN_API USnGA_DamageSkill : public USnGameplayAbility
{
	GENERATED_BODY()

protected:
	/** Shared damage scaling of the families: Damage x (Multiplier +
	 *  Level x PerLevelGrowth) - a FLAT addition to the multiplier, not a
	 *  compound factor on the base value, so each level adds exactly the
	 *  configured growth regardless of Multiplier. Equipment and skills
	 *  start at level 0: level 0 applies the multiplier as-is; a 0
	 *  PerLevelGrowth leaves the damage level-independent (the basic
	 *  attack). The caller passes its own field values: fields are
	 *  duplicated per family by design (see the class doc). */
	static float ScaleDamageByLevel(float Damage, float Multiplier, float PerLevelGrowth, int32 Level);

	/** The shared element clause of family descriptions: "X% 的<Element.*>元素
	 *  名</>伤害" - the final damage percentage at Level of the configured
	 *  element's damage, rendered with the element's rich-text style row. Each
	 *  family's GetDescription wraps this clause into its own sentence (sphere:
	 *  "...造成 {clause}，并将其击退"; projectile: "发射…，对命中的敌人造成
	 *  {clause}"). Keeps the panel/tooltip number identical to the damage
	 *  formula above - one formula, one text builder. */
	static FText BuildDamageClause(int32 Level, float Multiplier, float PerLevelGrowth, EDamageType DamageType);

	/** Where the strike goes out: the owning player's view yaw when the avatar is
	 *  player-controlled, otherwise the avatar's own facing. Read by the shape
	 *  cores as a VALUE - a strike that needs to land where the crosshair points
	 *  no longer has to turn the body to find out where that is.
	 *
	 *  Why the body turn went away (it used to be AlignAvatarToAimDirection): the
	 *  player character follows its movement direction, not the camera
	 *  (ASnCharacter - there is no turn-in-place animation to cover a
	 *  camera-driven spin), so a strike could either re-face the body or just read
	 *  the aim. Re-facing snaps the whole rig, and since the camera boom re-sets
	 *  its world rotation from the control rotation every tick
	 *  (USpringArmComponent::UpdateDesiredArmLocation, bUsePawnControlRotation)
	 *  a body turn that lands mid-frame drags the camera with it for that frame:
	 *  the jitter a projectile launch showed. The one role whose animation genuinely
	 *  wants the body turned asks for it per strike
	 *  (USnGA_SphereAttack::ShouldFaceAimOnStrike - the input basic attack).
	 *
	 *  AI avatars keep their own facing: enemy and companion abilities run through
	 *  these same cores, but their facing is their AI's business - an
	 *  AIController's control rotation would fight the yaw the enemy already set
	 *  for itself.
	 *
	 *  Yaw only: the sphere placement and the launch direction stay horizontal,
	 *  exactly as they were when the body was re-faced (a pawn's own pitch is 0),
	 *  so looking up or down never lifts a melee sphere off its plane.
	 *
	 *  Both call sites sit on the authority side (the montage hit frame is
	 *  authority-gated, the projectile arm is ServerOnly); a remote client's
	 *  control rotation reaches the server through its movement RPCs
	 *  (UCharacterMovementComponent::ServerMove_Implementation stores the view
	 *  rotation the client sends), so a guest's shots resolve from the same source
	 *  the old alignment baked into the body. */
	static FRotator GetAvatarAimRotation(const AActor* Avatar);
};
