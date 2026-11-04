// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGameplayEffectContext.h"

UScriptStruct* FSnGameplayEffectContext::GetScriptStruct() const
{
	return FSnGameplayEffectContext::StaticStruct();
}

FGameplayEffectContext* FSnGameplayEffectContext::Duplicate() const
{
	// Copy-construct: the base class members (instigator, source object, ...)
	// and the hit flags all copy over.
	return new FSnGameplayEffectContext(*this);
}

bool FSnGameplayEffectContext::NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess)
{
	// Base fields first (the engine's implementation, with its own bitmask).
	if (!FGameplayEffectContext::NetSerialize(Ar, Map, bOutSuccess))
	{
		return false;
	}

	// Then the hit flags. The element is a fixed small enum - one byte;
	// the bools serialize as one byte each.
	uint8 DamageTypeByte = static_cast<uint8>(DamageType);
	Ar << DamageTypeByte;
	if (Ar.IsLoading())
	{
		DamageType = static_cast<EDamageType>(DamageTypeByte);
	}
	Ar << bIsCriticalHit;
	Ar << bIsMissed;
	Ar << bIsReflected;

	return bOutSuccess;
}

FGameplayEffectContext* USnAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FSnGameplayEffectContext();
}
