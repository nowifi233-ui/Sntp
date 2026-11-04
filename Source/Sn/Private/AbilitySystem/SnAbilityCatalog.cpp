// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnAbilityCatalog.h"

const FSnAbilityInfo* USnAbilityCatalog::FindInfoByTag(const FGameplayTag& Tag) const
{
	return Abilities.FindByPredicate([&Tag](const FSnAbilityInfo& Info) { return Info.Tag == Tag; });
}

const FSnAbilityInfo* USnAbilityCatalog::FindInfoByClass(const TSubclassOf<USnGameplayAbility>& InAbilityClass) const
{
	return Abilities.FindByPredicate([&InAbilityClass](const FSnAbilityInfo& Info) { return Info.AbilityClass == InAbilityClass; });
}
