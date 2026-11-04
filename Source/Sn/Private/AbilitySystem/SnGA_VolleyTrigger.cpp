// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_VolleyTrigger.h"

USnGA_VolleyTrigger::USnGA_VolleyTrigger()
{
	// Per-use instance, matching the one-shot grant window it rides on: no
	// instance lingers between item uses (same reasoning as USnGA_OneShotAttack).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;

	// Release-and-finish defaults: three quick releases, then the ability ends
	// itself. Assets tune the count and rhythm on their class defaults.
	MaxTriggerCount = 3;
	IntervalSeconds = 0.1f;
}
