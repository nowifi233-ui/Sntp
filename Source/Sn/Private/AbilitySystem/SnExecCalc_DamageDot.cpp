// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnExecCalc_DamageDot.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Combat/USnCombatLibrary.h"

/** Captured attributes: the victim's three elemental resistances (DoT damage
 *  is always elemental - the igniting hit's element). The burning effect's
 *  Execution config lists them in the GE editor; the capture definitions must
 *  match exactly. */
struct FSnDamageDotStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IceResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);

	FSnDamageDotStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, FireResistance, Target, /*bSnapshot=*/false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, IceResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, LightningResistance, Target, false);
	}
};

static const FSnDamageDotStatics& DamageDotStatics()
{
	static FSnDamageDotStatics DotStatics;
	return DotStatics;
}

USnExecCalc_DamageDot::USnExecCalc_DamageDot()
{
	RelevantAttributesToCapture.Add(DamageDotStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageDotStatics().IceResistanceDef);
	RelevantAttributesToCapture.Add(DamageDotStatics().LightningResistanceDef);
}

void USnExecCalc_DamageDot::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	// The element flag rides on the context (set by the ignite GA when it
	// built the spec). Every context in this project is an
	// FSnGameplayEffectContext (see USnAbilitySystemGlobals), so the cast is
	// safe for effects built here.
	const FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();
	const FSnGameplayEffectContext* SnContext = static_cast<const FSnGameplayEffectContext*>(EffectContextHandle.Get());
	const EDamageType DamageType = SnContext ? SnContext->GetDamageType() : EDamageType::Fire;

	// Per-tick damage: the ignite GA wrote it (negative) under the element's
	// SetByCaller key; the spec persists across the duration's period ticks.
	float Damage = -Spec.GetSetByCallerMagnitude(USnCombatLibrary::GetDamageSetByCallerKey(DamageType), /*bWarnIfNotFound=*/false, 0.f);

	// Only the victim's elemental resistance mitigates a tick (no dodge/crit/
	// toughness/defense - those belonged to the hit that started the burn).
	if (Damage > 0.f)
	{
		float Resistance = 0.f;
		switch (DamageType)
		{
		case EDamageType::Fire:
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageDotStatics().FireResistanceDef, EvalParams, Resistance);
			break;
		case EDamageType::Ice:
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageDotStatics().IceResistanceDef, EvalParams, Resistance);
			break;
		case EDamageType::Lightning:
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageDotStatics().LightningResistanceDef, EvalParams, Resistance);
			break;
		default:
			break; // Non-elemental (physical) DoT has no resistance to apply.
		}
		Damage *= 1.f - FMath::Clamp(Resistance, 0.f, 1.f);
	}

	// Ship the resolved tick (positive) on the Damage meta attribute; the
	// victim's attribute set applies it to Health and broadcasts the result -
	// the normal damage chain, per tick.
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		USnAttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, Damage));
}
