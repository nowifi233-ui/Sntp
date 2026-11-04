// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_SummonCompanion.h"

FText USnGA_SummonCompanion::GetDescription_Implementation(int32 Level)
{
	// The companion's combat strength is its own (the companion blueprint
	// actor), so the description is static.
	return FText::FromString(TEXT("召唤一只同伴协助战斗，卸下装备后消失"));
}
