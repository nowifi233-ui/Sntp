// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

/**
 * Native gameplay tags. Declared here so C++ code has compile-time constants;
 * the same tags are used by data assets, effects and UI.
 */
namespace SnGameplayTags
{
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Health);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MaxHealth);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_AttackPower);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_XP);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Level);

	// --- Combat attributes (added with the numbers system) ---
	// Vital: mana pool + regeneration.
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MP);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MaxMP);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MPRegen);

	/** Health gained per period while the health regeneration effect is active
	 *  (the period and the coefficient live on the effect asset; the attribute
	 *  is only how much each tick adds). 0 = no regeneration. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_HealthRegen);

	/** Shield: an absorbing pool in FRONT of Health - incoming damage eats it
	 *  first and only the overflow reaches Health. Deliberately has no maximum
	 *  counterpart: skills and items simply ADD to it and the grants stack, so
	 *  how much shield a build can carry is a function of what it can cast, not
	 *  of a cap. Comes into existence at 0 and decays only by being hit (a grant
	 *  built on a duration GE makes it expire whole; see USnAttributeSet). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Shield);

	// Attack: physical + elemental base damage and their % amplifications.
	// The base attacks apply per-element values at a GE; the amplification
	// multiplies the same element's base at attack time (attacker side).
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PhysicalDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_PhysicalDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FireDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_IceDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_LightningDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FireDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_IceDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_LightningDamagePercent);

	/** Attacker-side flat reduction of the victim's Defense (effective defense =
	 *  max(0, Defense - Penetration)). Captured by USnExecCalc_Damage from the
	 *  attacker (the victim may not have the attacker's ASC around at resolution
	 *  time). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_ArmorPenetration);

	/** Attack tempo: cooldown scales as CD / (1 + AttackSpeed). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_AttackSpeed);

	/** Movement speed in cm/s; synced to CharacterMovement MaxWalkSpeed. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_MoveSpeed);

	// Defense: Defense = armor points (damage x K/(K+Defense)), Toughness = flat
	// reduction applied first.
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Defense);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Toughness);

	// Dodge: victim-side chance to ignore the entire hit (covers crits).
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Dodge);

	// Elemental resistances: victim-side % reduction per element.
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_FireResistance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_IceResistance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_LightningResistance);

	// Crit: chance and extra multiplier (total = 1 + CritDamage).
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CritChance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_CritDamage);

	/** Inventory slot count (raised by expansion items via GE modifiers). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attribute_Capacity);

	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Attack);

	/** Granted by the runtime-constructed difficulty effect while the player is
	 *  inside a combat zone. The effect is re-applied (old one removed by this
	 *  tag first) on every zone entry, so a change of difficulty never stacks. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Difficulty_Applied);

	/** Applied by the attack cooldown effect while the attack is on cooldown. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Attack);

	/** The enemy swing's own cooldown tag (GE_EnemyAttackCooldown grants it,
	 *  GA_EnemyAttack lists it in CooldownTags). Deliberately NOT
	 *  Cooldown.Attack: that one is the player input attack's identity and the
	 *  HUD's cooldown ring watches it. Note the GA's CooldownTags field is what
	 *  makes the tag gate anything - UGameplayAbility::CheckCooldown
	 *  short-circuits on an empty container, so a cooldown effect granting a tag
	 *  nobody lists blocks nothing. With it listed, the AI's per-frame
	 *  TryActivateAbility is silently refused while the tag is up, and that
	 *  refusal IS the swing pace (no timer anywhere). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_EnemyAttack);

	/** Granted by the item-use cooldown effect (GE_UseCooldown, see Sn
	 *  Settings) while a use is on cooldown. One active effect per item ID;
	 *  per-item existence is tracked by handle (USnInventoryComponent::
	 *  UseCooldownHandles) - this tag is the semantic marker for GE/UI queries
	 *  and dies with the effect on expiry. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cooldown_Use);

	/** Granted by the burning GE (the Fire bond's ignite, GE_Burn) while a
	 *  target is on fire. The ignite GA skips targets carrying it (one burn at
	 *  a time - no refresh), and the tag marks the state for GE queries and
	 *  future burning visuals. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Burning);

	/** Fired by EVERY primary-hit damage path once per damaged target: the
	 *  sphere core (USnGA_SphereAttack::ExecuteSphereAttack) and the
	 *  projectile impact (ASnProjectile, same block as its damage apply).
	 *  Payload.Target = the hit actor; the hit's element rides on the effect
	 *  context and Payload.EventMagnitude = the hit's damage. That number is
	 *  attacker-side and PRE-mitigation - what the swing was worth before the
	 *  victim's dodge / resistance / toughness / defense, the only value that
	 *  exists on the sending side (mitigation resolves on the victim, inside
	 *  the effect application, and nothing comes back from it) - so the proc
	 *  family's "share of the triggering amount" payload reads a magnitude the
	 *  attacker could have computed itself. Equipment-granted enchantment GAs
	 *  listen and append one independent element damage application; bond
	 *  passives (the ignite) filter by element. Discipline: SECONDARY damage
	 *  never fires it - enchantment appends and DoT ticks stay silent (an
	 *  enchant firing it would recurse into itself; a burn tick re-igniting
	 *  would refresh a burn mid-flight). New damage families fire it at their
	 *  primary-hit site, magnitude included.
	 *
	 *  Together with Event.DamageTaken this is the DAMAGE EVENT PAIR: the two
	 *  events whose effect context carries a real element, and therefore the only
	 *  two an element filter applies to (see USnGA_EventProc). The pair exists
	 *  because the two halves of a hit are known on different machines: this one
	 *  is what the attacker sent, that one is what the victim ended up taking. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttackHit);

	/** Fired by the attack montage at its hit frame (SnAnimNotify_GameplayEvent
	 *  placed on the swing montage) and consumed by the shared montage window
	 *  task (USnGA_SphereAttack::PlayAttackMontageAndWaitForWindow ->
	 *  OnAttackWindowEvent), which resolves the damage body there. A FIXED
	 *  native tag on purpose: the window is code, not per-skill data, and a
	 *  notify pointing at a tag no one listens to deals silent zero damage -
	 *  one tag, no field to get wrong. Every montage-driven attack role (the
	 *  player's input attack, the enemy swing) shares it. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttackWindow);

	/** Fired by USnAttributeSet when THIS actor resolves incoming damage, once per
	 *  damage application that landed (a fully avoided hit fires nothing). The
	 *  mirror of Event.AttackHit: that one is the attacker's view of the swing
	 *  before the victim's mitigation, this one is the victim's view of the
	 *  result - dodge / resistance / crit / toughness / defense are all already
	 *  in the number, because the victim is where they resolve
	 *  (USnExecCalc_Damage) and none of them ever travels back to the sender.
	 *
	 *  Payload.EventMagnitude = the damage the hit was worth, the SAME number the
	 *  floating damage number renders (`OnDamaged`). Shield absorption is
	 *  included: the pool sits in front of Health, so a hit it ate was still a
	 *  hit taken. Payload.Instigator = the attacker; the effect context carries
	 *  the element and the crit / miss flags.
	 *
	 *  Sent to the VICTIM's own ASC (HandleGameplayEvent, not a multicast): what
	 *  reacts to it - the proc family's "when I take damage" passives, i.e.
	 *  retaliation, resource gain on being hit, stacking buffs - is an ability on
	 *  that same ASC. Enemies carry the same attribute set and fire it too;
	 *  nothing listens unless something was granted to them. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_DamageTaken);

	/** Fired when an actor this character killed dies - sent to the KILLER's ASC,
	 *  the one event that crosses ability systems (USnAttributeSet looks the
	 *  killing blow's instigator up at the death latch). Fired once per death:
	 *  both death latches share the one-shot flag, and a burn / damage-over-time
	 *  kill credits the ignite's caster, the same instigator the XP drop and
	 *  OnDeath already use.
	 *
	 *  Payload.EventMagnitude = 0 ON PURPOSE: a kill has no number to scale a
	 *  payload by, and 0 makes a mistakenly configured ratio multiply to nothing
	 *  instead of producing a mystery value. Payload.Target = the victim; the
	 *  context is the killing blow's, so its element is the element of the last
	 *  hit (not of the fight). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_Kill);

	/** Fired by USnAttributeSet when a RESOURCE attribute actually rose, one
	 *  tag per attribute: Payload.EventMagnitude = what the effect really added
	 *  (after clamping - a heal that overshoots MaxHealth, or one refused by the
	 *  death latch, is worth less than it asked for, and a refused one worth
	 *  nothing, so the number is the applied delta, not the request). The event
	 *  goes to the owner's own ASC (HandleGameplayEvent, not a multicast cue) -
	 *  equipment passives that react to "I was healed / I gained shield" listen
	 *  here instead of having to watch the attribute.
	 *
	 *  Only INCREASES are fired: spending shield, an expiring temporary shield
	 *  and incoming damage all stay silent, so a listener never has to filter
	 *  by sign. Sent from PostGameplayEffectExecute, i.e. only effects that
	 *  EXECUTE (instant grants, heals, periodic regeneration ticks) qualify -
	 *  a bookkeeping write (revive writing MaxHealth, pool reuse, or an
	 *  equipment/difficulty effect re-evaluating the current value) does not
	 *  raise one.
	 *
	 *  ⚠️ A periodic heal fires one per tick, so a "heal -> shield" proc that
	 *  listens to Health scales with the tick rate; use the proc's own
	 *  EventsPerTrigger to thin it out. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttributeGained_Health);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttributeGained_Shield);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Event_AttributeGained_MP);

	/** Fired once per resolved hit (see ASnEnemy::HandleDamaged): the damage
	 *  number cue. GameplayCue events multicast to every client with the full
	 *  parameters - the damage magnitude (RawMagnitude.X) plus the hit's
	 *  element and crit/miss flags on the effect context - so every client
	 *  renders its own number. One cue per damage application: a frame with
	 *  two applications (base hit + enchantment bonus) delivers both, instead
	 *  of one result overwriting the other. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_DamageNumber);

	/** Fired when an enemy dies (see ASnEnemy::StartDeathSequence): the mesh
	 *  fall is a local-only transform (it never replicates), so clients replay
	 *  the fall locally when the cue arrives. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_EnemyDeathFall);

	/** Fired by the frost nova per sustain release (the sustain trigger's
	 *  child GA carries this tag as its SwingCueTag, and the sphere core
	 *  executes it once per release): the burst VFX notify (GC_AuraFrostBurst)
	 *  spawns the Niagara locally on every side - observers never run the
	 *  server-only child's instance, so the cue's multicast is the only path
	 *  that reaches them. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_AuraFrostBurst);

	/** Attack presentation cues, fired by the shared attack core
	 *  (USnGA_SphereAttack::ExecuteSphereAttack) - the GA itself never spawns
	 *  fx. The Swing cue fires once per activation, hit or miss: the notify
	 *  asset plays the weapon arc + whoosh at Params.Location facing
	 *  Params.Normal, plus the once-per-swing camera shake gated by
	 *  RawMagnitude > 0 (something was hit) and IsLocallyControlled. The Hit
	 *  cue fires once per damaged target (Params.Location = the target): the
	 *  notify plays the impact spark/sound. One pair per attack element -
	 *  Melee for the input basic attack, Fire for the one-shot fire skill. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Swing_Melee);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Melee);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Swing_Fire);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_Fire);

	/** The projectile family's impact cue (GA HitCueTag; the missile executes it
	 *  at the impact point when it damages an enemy). Separate from
	 *  GameplayCue.Hit.Fire so a fireball can show a big explosion while the
	 *  melee fire skill keeps its close-range spark - asset GC_Hit_FireProjectile
	 *  (GC_ + tag name, dots to underscores). Registered natively so GA
	 *  blueprints pick it from the tag picker. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayCue_Hit_FireProjectile);

	/** The summon ability (AbilityTags). The companion polls the owner for
	 *  this tag to know when the summon GA was revoked (unequip), so it must
	 *  be a registered native tag - use this one in the GA's AbilityTags. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Companion_Summon);

	/** Enemy team marker: carried by the ASC of anything hostile (enemies,
	 *  minions...). USnCombatLibrary::IsEnemy decides by this tag, not by
	 *  class. Child tags (e.g. Combat.Team.Enemy.Melee) count too. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Team_Enemy);

	/** Player team marker: carried by the player's ASC (added at construction,
	 *  like the enemy's). The mirror of Combat.Team.Enemy, and deliberately its
	 *  ONLY user - the enemy attack's target check
	 *  (USnGA_EnemyAttack::IsValidAttackTarget) reads it; no player-side query
	 *  does. Kept out of USnCombatLibrary because the enemy attack is currently
	 *  its single call site (promote it there when a second one appears, e.g. a
	 *  companion friendly-fire check): hostility stays one-directional per
	 *  query, and nothing carries both tags. Child tags count too
	 *  (HasMatchingGameplayTag). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Combat_Team_Player);

	/** The damage GE carries its element's magnitude under this key (one
	 *  element per hit - extra damage is a second application). Negative: the
	 *  GE adds it to the Damage meta attribute (GAS has no subtract op);
	 *  USnExecCalc_Damage reads it and resolves the mitigation. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Physical);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Fire);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Ice);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Lightning);

	/** Legacy keys - no longer written by the damage path (armor penetration is
	 *  captured by USnExecCalc_Damage, and the crit/miss flags ride on the
	 *  effect context). Kept registered so blueprint assets that still pick
	 *  them do not fail to compile. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Penetration);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage_Critical);

	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_XP);

	/** SetByCaller magnitude key for the item-use cooldown effect: the row's
	 *  UseCD (scaled by the player's CooldownReduction at use time) feeds the
	 *  GE's Duration magnitude, so the length is data-driven per item. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_UseCD);

	/** SetByCaller magnitude key for the burning GE's Duration (the ignite GA
	 *  writes its CDO's BurnDurationSeconds into it; the GE editor picks this
	 *  tag as the Duration magnitude). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_BurnDuration);

	/** SetByCaller magnitude keys for the generic item GEs (GE_Generic_*):
	 *  named after the attribute, applied with the item row's effective values.
	 *  Add one per attribute that items should modify. The GE editor's
	 *  SetByCaller tag picker only offers tags under the SetByCaller root. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Health);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MaxHealth);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AttackPower);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Capacity);

	// Per-attribute item keys for the combat attributes added with the
	// numbers system (same "SetByCaller.<AttributeName>" convention).
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MP);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MaxMP);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MPRegen);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_HealthRegen);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PhysicalDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_PhysicalDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_FireDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_IceDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_LightningDamage);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_FireDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_IceDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_LightningDamagePercent);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_ArmorPenetration);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_AttackSpeed);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_MoveSpeed);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Defense);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Toughness);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Dodge);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_FireResistance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_IceResistance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_LightningResistance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CritChance);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CritDamage);

	/** Shield granted by an item's use / equip modifiers (no cap - the row's
	 *  value is added as-is, so two sources simply stack). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Shield);

	/** Item stat key feeding the CooldownReduction attribute (point value,
	 *  10 = 10%); read at use time to shorten the item-use cooldown. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_CooldownReduction);

	/** Item capability tags (FSnItemRow::ItemTags). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Equipment);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Consumable);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Material);
	/** Grid-placement item that boosts the cells around it (see
	 *  FSnRuneShapeRow / FSnItemRow::RuneShapeID). */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Type_Rune);

	/** Bond (羁绊) tags (FSnItemRow::BondTags): item bond categories. The bond
	 *  data table rows use the tag names as row names (FSnBondRow - row name =
	 *  tag name), so items and tiers match by tag. Registered as native tags so
	 *  the item row's BondTags picker offers them instead of typing literals. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Bond_Food);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Bond_Pet);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Bond_Precision);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Bond_Burning);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Bond_Frozen);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Bond_Lightning);

	/** Slot context (right-click action filter): where the clicked slot lives.
	 *  Registered as native tags so DT_ItemActions's RequiredSource picks them
	 *  from the tag picker instead of typing literals. */
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Source_Bag);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Source_Equipment);
	SN_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Item_Source_Storage);

	/** Plain ACTOR tag (FName, NOT a registered gameplay tag - do not put it
	 *  in gameplay tag containers) marking chase targets. The player character
	 *  carries it; enemies acquire targets by scanning for it instead of
	 *  trusting GetPlayerController()->GetPawn() (which can be null right after
	 *  a level travel). Anything else that should draw aggro gets the tag too. */
	inline const FName Actor_Player = TEXT("Player");
}
