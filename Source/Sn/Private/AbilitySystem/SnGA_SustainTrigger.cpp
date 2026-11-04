// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_SustainTrigger.h"

#include "AbilitySystemComponent.h"

USnGA_SustainTrigger::USnGA_SustainTrigger()
{
	// Sustains ride the equip window, so one instance per equipped item - the
	// same instancing as the summon / enchantment abilities it replaces.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// The equip flow grants AND activates this (bActivateOnEquip is read by
	// USnItemLibrary::GrantEquipAbilities right after the grant) - the equip
	// window is the rhythm window. No OnAvatarSet self-activation: the engine's
	// OnAvatarSet timing (grant-time instance creation in 5.7) used to swallow
	// it, and the equip flow owns activation anyway.
	bActivateOnEquip = true;
}
