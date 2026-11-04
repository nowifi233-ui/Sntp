// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnAttributeCatalog.h"

const FSnAttributeInfo* USnAttributeCatalog::FindInfoByTag(const FGameplayTag& Tag) const
{
	for (const FSnAttributeInfo& Info : Attributes)
	{
		if (Info.Tag == Tag)
		{
			return &Info;
		}
	}
	return nullptr;
}

const FSnAttributeInfo* USnAttributeCatalog::FindInfoByAttribute(const FGameplayAttribute& Attribute) const
{
	if (!Attribute.IsValid())
	{
		return nullptr;
	}
	for (const FSnAttributeInfo& Info : Attributes)
	{
		if (Info.Attribute == Attribute)
		{
			return &Info;
		}
	}
	return nullptr;
}
