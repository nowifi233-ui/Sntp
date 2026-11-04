// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SnExecCalc_Damage.generated.h"

/**
 * Damage resolution for the damage gameplay effects (GE_AttackDamage,
 * GE_EnemyTouchDamage...). Runs on the server when the effect executes - the
 * effect itself only carries the element flag (on its context) and the
 * pre-mitigation magnitude (SetByCaller key).
 *
 * Resolution order (kept from the original attribute-set pipeline, so the
 * numbers are unchanged): victim dodge (full miss) -> elemental resistance
 * (physical has none) -> attacker crit roll x(1 + CritDamage) BEFORE the flat
 * reduction -> toughness (flat) -> defense (%, pierced by the attacker's
 * armor penetration). The final value is written on the Damage meta
 * attribute; USnAttributeSet applies it to Health and broadcasts the result
 * with the crit/miss flags from the context.
 */
UCLASS()
class SN_API USnExecCalc_Damage : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USnExecCalc_Damage();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
