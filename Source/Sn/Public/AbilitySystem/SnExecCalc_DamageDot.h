// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "SnExecCalc_DamageDot.generated.h"

/**
 * Damage resolution for damage-over-time gameplay effects (the burning GE
 * applied by the ignite bond passive, GE_Burn). Runs on the server on EVERY
 * period tick - the duration effect only carries the element flag (on its
 * context, set when the ignite GA built the spec) and the per-tick magnitude
 * (SetByCaller key).
 *
 * Deliberately lighter than USnExecCalc_Damage: a DoT tick is the repeated
 * application of one already-rolled event, so it skips the per-hit rolls that
 * happened (or should have happened) on the hit itself - no dodge, no crit,
 * no toughness/defense. Only the victim's elemental resistance applies:
 * damage x (1 - resistance). The final value is written on the Damage meta
 * attribute, so every tick inherits the normal damage chain (health clamp,
 * death latch, kill credit from the spec's instigator, damage numbers).
 *
 * The GE's Execution config must list exactly these three captures.
 */
UCLASS()
class SN_API USnExecCalc_DamageDot : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	USnExecCalc_DamageDot();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
