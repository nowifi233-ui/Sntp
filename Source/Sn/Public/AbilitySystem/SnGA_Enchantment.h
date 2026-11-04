// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/SnGA_DamageSkill.h"
#include "CoreMinimal.h"
#include "Combat/USnCombatLibrary.h"
#include "GameplayTagContainer.h"
#include "SnGA_Enchantment.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * Base class for equipment-granted enchantment abilities ("attack on-hit"
 * effects, e.g. a weapon adding 10% of the character's lightning damage to
 * each basic attack).
 *
 * Equip-flow passive: the item-equip grant path activates it once, right
 * after granting (bActivateOnEquip, read by USnItemLibrary::GrantEquipAbilities
 * - grant + activate in one, owned by the equip flow), and it stays alive for
 * the whole equip duration. While active it listens for the attack-hit event
 * (Event.AttackHit, fired by the attack GA per hit target) and appends ONE
 * independent damage application of DamageType, scaled by Multiplier and
 * PerLevelGrowth (the shared damage-family formula, ScaleDamageByLevel),
 * through the standard ExecCalc damage pipeline (element resistance, crit,
 * toughness, defense) - no per-hit ability activation cost.
 *
 * Inherits USnGA_DamageSkill only for the shared formula + description
 * clause: the enchantment has no damage shape of its own (the hit target
 * comes from the event), it is a third user of the same per-level growth.
 *
 * Unequipping revokes the ability, which cancels this activation and
 * unbinds the listener (EndAbility). Subclasses (or blueprint defaults)
 * pick the element / multiplier / damage effect class - a fire enchantment
 * is a duplicate blueprint with DamageType = Fire.
 */
UCLASS()
class SN_API USnGA_Enchantment : public USnGA_DamageSkill
{
	GENERATED_BODY()

public:
	USnGA_Enchantment();

	/** Description: the shared element clause (BuildDamageClause) at Level -
	 *  the effective percentage of the configured element's damage. */
	virtual FText GetDescription_Implementation(int32 Level) override;

	/** Element of the appended hit (single-element model: one enchantment
	 *  appends exactly one damage application per attack hit). */
	UPROPERTY(EditDefaultsOnly, Category = "Enchant")
	EDamageType DamageType = EDamageType::Lightning;

	/** Appended damage = the source's damage of DamageType (base x (1 +
	 *  percent)) times this multiplier (0.1 = 10% of the element's damage). */
	UPROPERTY(EditDefaultsOnly, Category = "Enchant", meta = (ClampMin = "0.0"))
	float Multiplier = 0.1f;

	/** Multiplier added per ability level - a FLAT addition to the
	 *  multiplier, not a compound factor on the source damage: effective
	 *  multiplier = Multiplier + Level x PerLevelGrowth. Equipment levels
	 *  start at 0, so level 0 (the base item level) applies the base
	 *  Multiplier alone and each level adds exactly the configured growth; a
	 *  0 keeps the appended hit level-independent. Same field/convention as
	 *  the damage families (USnGA_DamageSkill::ScaleDamageByLevel). */
	UPROPERTY(EditDefaultsOnly, Category = "Enchant", meta = (ClampMin = "0.0"))
	float PerLevelGrowth = 0.f;

	/** The damage GE carrying USnExecCalc_Damage (e.g. GE_AttackDamage). */
	UPROPERTY(EditDefaultsOnly, Category = "Enchant")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

protected:
	/** Binds the attack-hit listener for as long as the ability is active. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Unbinds the listener (cancelled on unequip / level travel). */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Event.AttackHit arrived: append the enchantment hit to the target. */
	void HandleAttackEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

private:
	/** Registered with EventFilter in ActivateAbility; removed (by the same
	 *  filter) in EndAbility. */
	FDelegateHandle EventDelegateHandle;

	/** The tag container the listener was registered with -
	 *  RemoveGameplayEventTagContainerDelegate matches by the same filter. */
	FGameplayTagContainer EventFilter;
};
