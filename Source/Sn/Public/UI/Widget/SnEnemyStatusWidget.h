// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SnEnemyStatusWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnHealthPercentChangedDelegate, float, Percent);

/**
 * C++ base for the enemy overhead widget (WBP_EnemyStatus).
 * The enemy drives it through these events; the blueprint owns the visuals:
 * level text (always), health bar (combat only).
 *
 * Deliberately minimal for the health bar: the C++ side only stores the
 * target percent and broadcasts it on change. The delayed-damage "ghost bar"
 * animation lives entirely in the blueprint's Event Tick (interpolate a
 * local ghost value toward GetTargetHealthPercent) - the widget's tick runs
 * (ASnEnemy forces the WidgetComponent to ETickMode::Enabled), and keeping
 * the animation in the blueprint makes delay/speed tweakable without C++.
 */
UCLASS(Abstract, Blueprintable)
class SN_API USnEnemyStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the enemy's level (always visible). */
	UFUNCTION(BlueprintImplementableEvent, Category = "EnemyStatus")
	void SetLevel(int32 InLevel);

	/** Stores the new target fill and broadcasts it (called by ASnEnemy on
	 *  every health change). */
	UFUNCTION(BlueprintCallable, Category = "EnemyStatus")
	void UpdateHealthPercent(float Percent);

	/** Shows/hides the health bar (combat mode driven by the local player). */
	UFUNCTION(BlueprintImplementableEvent, Category = "EnemyStatus")
	void SetHealthBarVisible(bool bVisible);

	/** Fired whenever the target health changes; bind it to set the main bar
	 *  immediately. */
	UPROPERTY(BlueprintAssignable, Category = "EnemyStatus")
	FSnHealthPercentChangedDelegate OnHealthPercentChanged;

	/** The current target fill (0..1). The blueprint's Event Tick reads this
	 *  to animate its ghost bar (and its initial setup reads it for the first
	 *  frame). */
	UFUNCTION(BlueprintPure, Category = "EnemyStatus")
	float GetTargetHealthPercent() const { return TargetPercent; }

private:
	float TargetPercent = 1.f;
};
