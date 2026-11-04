// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnExecCalc_Damage.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Combat/USnCombatLibrary.h"
#include "Game/SnSettings.h"

/** Captured attributes: attacker-side (damage numbers) and victim-side
 *  (mitigation). The damage effect's Execution config lists them in the GE
 *  editor; the capture definitions must match exactly. */
struct FSnDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(ArmorPenetration);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritChance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CritDamage);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Dodge);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Toughness);
	DECLARE_ATTRIBUTE_CAPTUREDEF(FireResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(IceResistance);
	DECLARE_ATTRIBUTE_CAPTUREDEF(LightningResistance);

	FSnDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, ArmorPenetration, Source, /*bSnapshot=*/false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, CritChance, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, CritDamage, Source, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, Dodge, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, Defense, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, Toughness, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, FireResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, IceResistance, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USnAttributeSet, LightningResistance, Target, false);
	}
};

static const FSnDamageStatics& DamageStatics()
{
	static FSnDamageStatics DStatics;
	return DStatics;
}

USnExecCalc_Damage::USnExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().ArmorPenetrationDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritChanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().CritDamageDef);
	RelevantAttributesToCapture.Add(DamageStatics().DodgeDef);
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);
	RelevantAttributesToCapture.Add(DamageStatics().ToughnessDef);
	RelevantAttributesToCapture.Add(DamageStatics().FireResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().IceResistanceDef);
	RelevantAttributesToCapture.Add(DamageStatics().LightningResistanceDef);
}

void USnExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = SourceTags;
	EvalParams.TargetTags = TargetTags;

	// The element flag rides on the context (set by USnCombatLibrary). Every
	// context in this project is an FSnGameplayEffectContext (see
	// USnAbilitySystemGlobals), so the cast is safe for effects built here.
	// Copy the handle: the original is const, but the flags below are written
	// into the shared context object for the attribute set to read.
	FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();
	FSnGameplayEffectContext* SnContext = static_cast<FSnGameplayEffectContext*>(EffectContextHandle.Get());
	const EDamageType DamageType = SnContext ? SnContext->GetDamageType() : EDamageType::Physical;

	// Pre-mitigation damage: the GE wrote it (negative) under the element's
	// SetByCaller key.
	float Damage = -Spec.GetSetByCallerMagnitude(USnCombatLibrary::GetDamageSetByCallerKey(DamageType), /*bWarnIfNotFound=*/false, 0.f);

	// 1. Victim dodge: ignores the whole hit (covers crits).
	float Dodge = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DodgeDef, EvalParams, Dodge);
	const bool bMissed = Damage > 0.f && FMath::FRand() < FMath::Clamp(Dodge, 0.f, 1.f);
	if (SnContext)
	{
		SnContext->SetIsMissed(bMissed);
	}
	if (bMissed)
	{
		Damage = 0.f;
	}

	// 2. Elemental resistance (physical has none - defense is its counter).
	if (Damage > 0.f)
	{
		float Resistance = 0.f;
		switch (DamageType)
		{
		case EDamageType::Fire:
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().FireResistanceDef, EvalParams, Resistance);
			break;
		case EDamageType::Ice:
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().IceResistanceDef, EvalParams, Resistance);
			break;
		case EDamageType::Lightning:
			ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().LightningResistanceDef, EvalParams, Resistance);
			break;
		default:
			break; // Physical
		}
		Damage *= 1.f - FMath::Clamp(Resistance, 0.f, 1.f);
	}

	// 3. Attacker crit roll. Rolled BEFORE the flat toughness reduction so a
	// crit multiplies the pre-toughness damage - the same result the original
	// attacker-side roll produced (crit applied before mitigation).
	float CritChance = 0.f;
	float CritDamage = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritChanceDef, EvalParams, CritChance);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CritDamageDef, EvalParams, CritDamage);
	const bool bCritical = Damage > 0.f && FMath::FRand() < FMath::Clamp(CritChance, 0.f, 1.f);
	if (SnContext)
	{
		SnContext->SetIsCriticalHit(bCritical);
	}
	if (bCritical)
	{
		Damage *= 1.f + FMath::Max(CritDamage, 0.f);
	}

	// 4. Toughness (flat), then defense (armor curve), defense pierced by the
	// attacker's armor penetration. Flat before percentage is the order every
	// mainstream ARPG uses - the two never interact.
	if (Damage > 0.f)
	{
		float Toughness = 0.f;
		float Defense = 0.f;
		float Penetration = 0.f;
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ToughnessDef, EvalParams, Toughness);
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvalParams, Defense);
		ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorPenetrationDef, EvalParams, Penetration);

		// Standard armor curve (the LoL/Dota family): mitigation is D/(D+K), so
		// the damage multiplier is K/(K+D). Diminishing returns come free - every
		// point of defense buys the same slice of effective health, so there is
		// no cap to keep, and with D >= 0 the factor stays inside (0, 1]: damage
		// floors at "nearly nothing" and can never turn negative. The old linear
		// "1 - Defense" form needed a percentage defense and a clamp to survive;
		// an out-of-range base inverted the factor and healed the victim.
		const float K = FMath::Max(1.f, USnSettings::Get()->GetDefenseConstant());
		const float EffectiveDefense = FMath::Max(0.f, Defense - Penetration);
		Damage = FMath::Max(0.f, Damage - FMath::Max(Toughness, 0.f)) * (K / (K + EffectiveDefense));
	}

	// Ship the resolved damage (positive) on the Damage meta attribute; the
	// victim's attribute set applies it to Health.
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		USnAttributeSet::GetDamageAttribute(), EGameplayModOp::Additive, Damage));
}
