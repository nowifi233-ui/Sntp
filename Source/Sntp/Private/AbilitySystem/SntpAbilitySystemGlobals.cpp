// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SntpAbilitySystemGlobals.h"

#include "SntpAbilityTypes.h"

FGameplayEffectContext* USntpAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSntpGameplayEffectContext();
}
