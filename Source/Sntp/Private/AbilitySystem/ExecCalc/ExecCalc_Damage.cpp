// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"

#include "SntpGameplayTags.h"
#include "AbilitySystem/SntpAttributeSet.h"

struct SntpDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	SntpDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USntpAttributeSet, MaxHealth, Target, false);
	}
};

static const SntpDamageStatics& DamageStatics()
{
	static SntpDamageStatics DStatics;
	return DStatics;
}

UExecCalc_Damage::UExecCalc_Damage()
{
	RelevantAttributesToCapture.Add(DamageStatics().MaxHealthDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                              FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
	
	FGameplayEffectSpec Spec = ExecutionParams.GetOwningSpec();
	
	float Damage = Spec.GetSetByCallerMagnitude(FSntpGameplayTags::Get().Damage);
	
	FGameplayModifierEvaluatedData EvaluatedData(USntpAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
}
