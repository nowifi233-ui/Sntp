// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/ExecCalc/ExecCalc_Damage.h"
#include "SntpAbilityTypes.h"
#include "SntpGameplayTags.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Evaluation/IMovieSceneEvaluationHook.h"

struct SntpDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(MaxHealth);
	DECLARE_ATTRIBUTE_CAPTUREDEF(CriticalChance);
	SntpDamageStatics()
	{
		DEFINE_ATTRIBUTE_CAPTUREDEF(USntpAttributeSet, MaxHealth, Target, false);
		DEFINE_ATTRIBUTE_CAPTUREDEF(USntpAttributeSet, CriticalChance, Target, false);
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
	RelevantAttributesToCapture.Add(DamageStatics().CriticalChanceDef);
}

void UExecCalc_Damage::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
                                              FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	Super::Execute_Implementation(ExecutionParams, OutExecutionOutput);
	
	FGameplayEffectSpec Spec = ExecutionParams.GetOwningSpec();
	
	FGameplayEffectContextHandle EffectContextHandle = Spec.GetContext();
	FGameplayEffectContext* Context = EffectContextHandle.Get();
	FSntpGameplayEffectContext* SntpContext = static_cast<FSntpGameplayEffectContext*>(Context);
	
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
	
	float CriticalChance = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().CriticalChanceDef, EvaluationParameters, CriticalChance);
	
	float Damage = Spec.GetSetByCallerMagnitude(FSntpGameplayTags::Get().Damage);
	if (const int32 Temp = FMath::RandRange(0, 100); Temp < CriticalChance)
	{
		Damage = Damage * 2.f;
		SntpContext->SetIsCritical(true);
	}
	else
	{
		SntpContext->SetIsCritical(false);
	}
	
	float Resilience = Spec.GetSetByCallerMagnitude(FSntpGameplayTags::Get().Resilience);
	
	FGameplayModifierEvaluatedData EvaluatedData(USntpAttributeSet::GetIncomingDamageAttribute(), EGameplayModOp::Additive, Damage);
	OutExecutionOutput.AddOutputModifier(EvaluatedData);
	
	FGameplayModifierEvaluatedData EvaluatedData1(USntpAttributeSet::GetResilienceAttribute(), EGameplayModOp::Additive, -Resilience);
	OutExecutionOutput.AddOutputModifier(EvaluatedData1);
}
