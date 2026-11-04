// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "AbilitySystem/SnGameplayAbility.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SnGA_Ignite.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
struct FGameplayEventData;

/**
 * Ignite-on-fire-hit passive, the Fire bond's tier ability (FSnBondTier::
 * EquipAbility, granted + activated by USnInventoryComponent::RecomputeBonds
 * through the same flow as the item rows' EquipAbilities - grant + activate
 * in one via bActivateOnEquip; losing the tier revokes it, which cancels this
 * activation and unbinds the listener).
 *
 * While active it listens for the attack-hit event (Event.AttackHit, fired by
 * the shared attack core USnGA_SphereAttack::ExecuteSphereAttack once per
 * damaged target - the sphere family only; projectile hits do not fire the
 * event yet). When the hit's element is Fire, the target is ignited with
 * IgniteChance: the burning GE (IgniteEffectClass - Duration SetByCaller +
 * Period + granted Status.Burning, execution = USnExecCalc_DamageDot) is
 * applied to the victim with the attacker as instigator, so every burn tick
 * inherits the whole damage chain (fire damage numbers, kill credit).
 *
 * A target already burning (Status.Burning granted by the GE) is NOT
 * re-ignited: the DoT runs its course, then later fire hits can ignite again.
 * Same shape as USnGA_Enchantment - InstancedPerActor + one delegate listener
 * bound in ActivateAbility, so a hit never pays a per-activation cost. Values
 * live on the CDO; per-tier strength is a per-tier subclass asset.
 */
UCLASS()
class SN_API USnGA_Ignite : public USnGameplayAbility
{
	GENERATED_BODY()

public:
	USnGA_Ignite();

	/** Description: chance + per-tick damage + duration (all CDO fields, no
	 *  level scaling). */
	virtual FText GetDescription_Implementation(int32 Level) override;

	/** Per Fire-hit ignite chance (0-1). */
	UPROPERTY(EditDefaultsOnly, Category = "Ignite", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float IgniteChance = 0.25f;

	/** Burning damage PER TICK (positive; the GE's Period decides the tick
	 *  rate). Written as the element's negative SetByCaller, which the dot
	 *  ExecCalc reads on every period. */
	UPROPERTY(EditDefaultsOnly, Category = "Ignite", meta = (ClampMin = "0.0"))
	float TickDamage = 0.f;

	/** How long the burning GE lasts (written into its SetByCaller Duration). */
	UPROPERTY(EditDefaultsOnly, Category = "Ignite", meta = (ClampMin = "0.0"))
	float BurnDurationSeconds = 3.f;

	/** The burning GE asset (Duration SetByCaller + Period + granted
	 *  Status.Burning, execution = USnExecCalc_DamageDot). */
	UPROPERTY(EditDefaultsOnly, Category = "Ignite")
	TSubclassOf<UGameplayEffect> IgniteEffectClass;

protected:
	/** Binds the attack-hit listener for as long as the ability is active. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Unbinds the listener (revoked on tier loss / level travel). */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Event.AttackHit arrived: if the hit was Fire, roll the chance and
	 *  ignite an unburning target. */
	void HandleAttackEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

private:
	/** Registered with EventFilter in ActivateAbility; removed (by the same
	 *  filter) in EndAbility. */
	FDelegateHandle EventDelegateHandle;

	/** The tag container the listener was registered with -
	 *  RemoveGameplayEventTagContainerDelegate matches by the same filter. */
	FGameplayTagContainer EventFilter;
};
