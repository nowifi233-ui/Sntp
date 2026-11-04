// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
// FGameplayTag (SendAttributeGainedEvent) + FGameplayEffectSpec (via
// GameplayEffect.h inside USnCombatLibrary.h) for the gained-event sender.
#include "GameplayTagContainer.h"
#include "Combat/USnCombatLibrary.h"
#include "SnAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnHealthChangedDelegate, float, NewHealth);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnDeathDelegate, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FSnDamageTakenDelegate, float, FinalDamage, bool, bCritical, bool, bMissed, EDamageType, DamageType);

/**
 * Core attribute set: resources (health/mana), combat numbers (damage,
 * defense, crit, dodge, attack/move speed) and leveling (XP / Level).
 *
 * Damage pipeline (Aura-style meta attribute + ExecCalc resolution):
 * attackers write one element's magnitude on the Damage meta attribute (GE
 * modifier, negative - set by USnCombatLibrary) and carry the element flag on
 * the effect context. USnExecCalc_Damage resolves mitigation when the GE
 * executes (dodge -> resistance -> crit -> toughness -> defense), writing the
 * final amount back on the meta attribute; PostGameplayEffectExecute only
 * applies it to Health and broadcasts the result with the crit/miss flags.
 * See Docs/DEVELOPMENT_LOG.md (numbers system entry).
 */
UCLASS()
class SN_API USnAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	USnAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// --- Vital resources ---
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Health);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, MaxHealth);

	/** Health gained per period while the health regeneration effect is active
	 *  (see ASnPlayerState::HealthRegenEffectClass - the period and the
	 *  coefficient live on the effect asset, this is only how much one tick
	 *  adds). Comes into existence at 0 and is only ever raised by gameplay
	 *  effects: equipment and skills add to it exactly like MPRegen, so a
	 *  "regen per second" stat is one row in the generic equip effect.
	 *
	 *  Regeneration stops at zero health - the death latch refuses Health
	 *  increases while it is set, so a corpse cannot walk itself back up and
	 *  silently eat the next death (see PreAttributeChange). Clamped at 0 as
	 *  well: negative regen would be a slow death with no kill credit, which is
	 *  what the damage-over-time lane is for. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_HealthRegen)
	FGameplayAttributeData HealthRegen;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, HealthRegen);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MP)
	FGameplayAttributeData MP;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, MP);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxMP)
	FGameplayAttributeData MaxMP;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, MaxMP);

	/** Mana gained per second while the mana regen effect is active. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MPRegen)
	FGameplayAttributeData MPRegen;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, MPRegen);

	/** Absorbing pool sitting IN FRONT of Health: incoming damage eats it first
	 *  and only the overflow reaches Health (resolved in
	 *  PostGameplayEffectExecute - the last stop of the damage pipeline, after
	 *  USnExecCalc_Damage has finished dodge / resistance / crit / toughness /
	 *  defense, so shield never interacts with any of them).
	 *
	 *  No maximum counterpart, by design: grants ADD to it and stack, so how
	 *  much a build can carry is a function of what it can cast, not of a cap.
	 *  It only ever goes down by being hit.
	 *
	 *  Grants MUST be Instant effects on AddBase: those write the base value
	 *  directly, which is what makes spending exact. A Has Duration / Infinite
	 *  effect on Shield instead registers an aggregator modifier (engine:
	 *  non-instant, non-periodic modifiers go to the aggregator) that is added
	 *  on top of every base write - the pool would GROW each time it absorbs a
	 *  hit. A temporary shield is therefore two Instant grants from one
	 *  ability: +N when it is cast, -N when it lapses (a partly spent pool
	 *  clamps at 0 in PreAttributeChange, so the spent part is not refunded).
	 *  Starts at 0 and is never granted by the attribute set itself. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Shield);

	// --- Damage numbers ---
	/** Physical base damage (the player attack / enemy touch damage). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_PhysicalDamage)
	FGameplayAttributeData PhysicalDamage;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, PhysicalDamage);

	/** Physical amplification: physical damage x (1 + percent). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_PhysicalDamagePercent)
	FGameplayAttributeData PhysicalDamagePercent;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, PhysicalDamagePercent);

	/** Elemental base damages: consumed only by skills / bonus damage of the
	 *  matching element (a plain attack deals physical only). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_FireDamage)
	FGameplayAttributeData FireDamage;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, FireDamage);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_IceDamage)
	FGameplayAttributeData IceDamage;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, IceDamage);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_LightningDamage)
	FGameplayAttributeData LightningDamage;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, LightningDamage);

	/** Elemental amplifications: same-element base x (1 + percent). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_FireDamagePercent)
	FGameplayAttributeData FireDamagePercent;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, FireDamagePercent);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_IceDamagePercent)
	FGameplayAttributeData IceDamagePercent;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, IceDamagePercent);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_LightningDamagePercent)
	FGameplayAttributeData LightningDamagePercent;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, LightningDamagePercent);

	// --- Attack tempo / movement ---
	/** Cooldown scales as CD / (1 + AttackSpeed). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackSpeed)
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, AttackSpeed);

	/** Movement speed in cm/s; synced to the character's MaxWalkSpeed. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, MoveSpeed);

	// --- Defense ---
	/** Armor points: incoming damage is scaled by K / (K + Defense), K being
	 *  USnSettings::DefenseConstant (100 by default - 100 Defense halves the
	 *  damage, 300 keeps a quarter of it). The curve carries its own diminishing
	 *  returns, so there is no cap to tune; clamped at >= 0 only. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Defense)
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Defense);

	/** Flat damage reduction applied before the percentage defense. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Toughness)
	FGameplayAttributeData Toughness;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Toughness);

	/** Attacker-side flat reduction of the victim's Defense (the curve reads
	 *  max(0, Defense - ArmorPenetration)). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ArmorPenetration)
	FGameplayAttributeData ArmorPenetration;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, ArmorPenetration);

	// --- Crit ---
	/** Chance to critically hit (0..1). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CritChance)
	FGameplayAttributeData CritChance;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, CritChance);

	/** Extra crit multiplier: a crit deals x (1 + CritDamage). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CritDamage)
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, CritDamage);

	// --- Cooldown reduction (item-use cooldown stat) ---
	/** Point value in [0, 100] (10 = 10%). Shortens the item-use cooldown at
	 *  use time: EffectiveCD = UseCD x (1 - CooldownReduction / 100), clamped
	 *  to a 0.1s floor. Only cooldowns STARTED while the stat is in effect
	 *  benefit - a running cooldown keeps the length it started with. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_CooldownReduction)
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, CooldownReduction);

	// --- Dodge ---
	/** Victim-side chance to ignore the whole hit (covers crits), 0..1. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Dodge)
	FGameplayAttributeData Dodge;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Dodge);

	// --- Elemental resistances (victim-side % per element) ---
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_FireResistance)
	FGameplayAttributeData FireResistance;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, FireResistance);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_IceResistance)
	FGameplayAttributeData IceResistance;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, IceResistance);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_LightningResistance)
	FGameplayAttributeData LightningResistance;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, LightningResistance);

	// --- Meta attribute (NOT replicated, NOT in the attribute catalog) ---
	/** Carrier of incoming damage: the damage GE's execution (USnExecCalc_
	 *  Damage) writes the resolved amount here (positive, via the effect's
	 *  Executions array); PostGameplayEffectExecute applies it to Health and
	 *  resets this to 0. Never displayed. */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Damage);

	// --- Leveling (unchanged) ---
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_XP)
	FGameplayAttributeData XP;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, XP);

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Level)
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Level);

	/** Inventory slot count; expansion items raise it via GE modifiers
	 *  (Instant = permanent, Infinite = while equipped). */
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Capacity)
	FGameplayAttributeData Capacity;
	ATTRIBUTE_ACCESSORS(USnAttributeSet, Capacity);

	/** XP needed to advance from the given level to the next (level * 100). */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	static float GetXPRequiredForLevel(int32 CurrentLevel);

	/** Broadcast whenever Health changes from a gameplay effect. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnHealthChangedDelegate OnHealthChanged;

	/** Broadcast when incoming damage is RESOLVED (by USnExecCalc_Damage,
	 *  after dodge/resist/crit/toughness/defense), with the final damage
	 *  actually dealt (0 on a dodge) and the crit/miss flags from the effect
	 *  context. Damage numbers render from this, so crits and misses get
	 *  distinct styling. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnDamageTakenDelegate OnDamaged;

	/** Broadcast once when Health reaches zero, with the killer (may be null). */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnDeathDelegate OnDeath;

	/** Re-arms the one-shot death flag (pooled enemies must be able to die again). */
	void ResetDeath() { bIsDead = false; }

protected:
	/** Sends a gameplay event to TargetActor's ability system component. The one
	 *  implementation behind every event this attribute set raises: the payload
	 *  is assembled from Spec's effect context (instigator, source object,
	 *  context handle - so a listener can see who caused it and with what
	 *  element) plus the given magnitude, and the event is handed to the target's
	 *  ASC directly (HandleGameplayEvent, no multicast cue): whatever reacts to
	 *  it is an ability on that same ASC, and what it does in response replicates
	 *  on its own.
	 *
	 *  Authority-only (these events are raised from effect execution, which is
	 *  server-side anyway - the guard just makes it explicit) and silent when the
	 *  target has no ASC or is already gone (a killer who logged out mid-swing).
	 *
	 *  TargetActor is the actor whose ASC should hear the event: this actor for
	 *  the self-events (Event.DamageTaken, Event.AttributeGained.*), and the
	 *  killer for Event.Kill - the single hop that crosses to another ASC. */
	void SendGameplayEvent(AActor* TargetActor, const FGameplayTag& EventTag, float Magnitude, const FGameplayEffectSpec& Spec) const;

	/** Fires one of the Event.AttributeGained.* tags at the owning ASC when an
	 *  effect actually RAISED a resource attribute. Ignores zero/negative
	 *  deltas (spending shield and expiry are subtractions). Thin wrapper over
	 *  SendGameplayEvent - the sign gate is the only thing it adds.
	 *  Amount = the real delta, Spec = the effect that caused it (instigator /
	 *  source object ride along for listeners and debugging). */
	void SendAttributeGainedEvent(const FGameplayTag& EventTag, float Amount, const FGameplayEffectSpec& Spec);

	/** Pushes MoveSpeed to the owning character's CharacterMovement MaxWalkSpeed.
	 *  Called from PostAttributeChange, which fires on both the server (GE
	 *  application) and clients (attribute replication) - movement is locally
	 *  driven on each client and server-authoritative on the server. */
	void SyncMoveSpeed(float NewValue);

	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_HealthRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MP(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxMP(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MPRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Shield(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PhysicalDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PhysicalDamagePercent(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FireDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_IceDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LightningDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FireDamagePercent(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_IceDamagePercent(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LightningDamagePercent(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_AttackSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MoveSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Toughness(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritChance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CritDamage(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Dodge(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_FireResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_IceResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LightningResistance(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_XP(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Level(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Capacity(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_CooldownReduction(const FGameplayAttributeData& OldValue);

	/** True once Health hit zero, so OnDeath only fires for the first (lethal) hit. */
	bool bIsDead = false;

	/** Current value of a watched resource attribute (Health / Shield / MP),
	 *  snapshotted in PreAttributeChange - the last moment before a write where
	 *  the old value is still readable. PostGameplayEffectExecute (where the
	 *  Event.AttributeGained.* events are raised) runs AFTER the engine wrote
	 *  the new value and is never handed the old one, and the effect's
	 *  requested magnitude is not the applied one either (a heal over MaxHealth,
	 *  or one the death latch refused, is worth less or nothing), so the only
	 *  honest source for "how much did this effect really add" is this
	 *  before-the-write look.
	 *
	 *  ONE member for all three: each branch of PostGameplayEffectExecute
	 *  copies it into a local as its FIRST statement, before anything that can
	 *  nest a new effect application (raising the event runs proc payloads,
	 *  which apply effects) and thereby overwrite it. */
	float PreWriteValue = 0.f;
};
