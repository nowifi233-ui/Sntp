// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/SnWidgetController.h"
#include "SnCombatWidgetController.generated.h"

struct FOnAttributeChangeData;
struct FGameplayTag;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnAttributeFloatChangedDelegate, float, NewValue);

/**
 * Broadcasts combat attributes to the HUD. One controller for everything
 * that reads player combat stats (health/MP bars, damage display, etc.).
 * Stats-panel rows are catalog-driven and do NOT need delegates here - the
 * four-step registration rule only requires controller delegates for
 * HUD-level readouts.
 */
UCLASS(BlueprintType)
class SN_API USnCombatWidgetController : public USnWidgetController
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnMaxHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnPhysicalDamageChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnMPChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnMaxMPChanged;

	/** Mana gained per second; drives the regen readout next to the MP bar. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnMPRegenChanged;

	/** Shield pool in front of Health: rises when a skill grants it, falls as
	 *  it absorbs hits. Uncapped, so the bar is drawn against MaxHealth (see
	 *  GetShieldPercent) rather than against a maximum of its own. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnShieldChanged;

	/** Derived value: Health / MaxHealth in [0, 1]. Broadcast whenever either changes. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnHealthPercentChanged;

	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnXPChanged;

	/** Current level (as float; round in UI for display). */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnLevelChanged;

	/** Derived value: XP / XP required for the current level in [0, 1]. Broadcast whenever either changes. */
	UPROPERTY(BlueprintAssignable, Category = "Attributes")
	FSnAttributeFloatChangedDelegate OnXPPercentChanged;

	/** Attack cooldown state: cooldown seconds when it starts, 0 when it ends. */
	UPROPERTY(BlueprintAssignable, Category = "Cooldown")
	FSnAttributeFloatChangedDelegate OnAttackCooldownChanged;

	/** Pull: current Health / MaxHealth in [0, 1]. Widgets can call this on init as a fallback to the push. */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealthPercent() const;

	/** Pull: current attribute values, fallback to the push delegates. */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetPhysicalDamage() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMP() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMaxMP() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMPRegen() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetShield() const;

	/** Shield / MaxHealth - the shield bar's fill, since the pool has no
	 *  maximum of its own. Computed on read: refresh it from BOTH
	 *  OnShieldChanged and OnMaxHealthChanged (MaxHealth grows on level-up,
	 *  which rescales the bar's denominator). */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetShieldPercent() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetXP() const;

	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetLevel() const;

	/** XP needed to reach the next level from the current level. */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetMaxXP() const;

	/** Current XP / XP required for the current level in [0, 1]. */
	UFUNCTION(BlueprintPure, Category = "Attributes")
	float GetXPPercent() const;

protected:
	virtual void BroadcastInitialValues() override;
	virtual void BindCallbacks() override;

private:
	void HealthChangedCallback(const FOnAttributeChangeData& Data);
	void MaxHealthChangedCallback(const FOnAttributeChangeData& Data);
	void PhysicalDamageChangedCallback(const FOnAttributeChangeData& Data);
	void MPChangedCallback(const FOnAttributeChangeData& Data);
	void MaxMPChangedCallback(const FOnAttributeChangeData& Data);
	void MPRegenChangedCallback(const FOnAttributeChangeData& Data);
	void ShieldChangedCallback(const FOnAttributeChangeData& Data);
	void XPChangedCallback(const FOnAttributeChangeData& Data);
	void LevelChangedCallback(const FOnAttributeChangeData& Data);

	/** Cooldown.Attack tag gained/lost: drives the attack cooldown ring. */
	void AttackCooldownTagChanged(const FGameplayTag Tag, int32 NewCount);

	/** Recomputes Health / MaxHealth and broadcasts it. */
	void BroadcastHealthPercent() const;

	/** Recomputes XP / XP required for the current level and broadcasts it. */
	void BroadcastXPPercent() const;
};
