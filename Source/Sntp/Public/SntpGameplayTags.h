// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Singleton containing native gameplay tags.
 */

struct FSntpGameplayTags
{
public:
	static const FSntpGameplayTags& Get() {return GameplayTags;}
	static void InitializeNativeGameplayTags();
	
	FGameplayTag Attributes_Vital_MaxHealth;
	FGameplayTag Attributes_Vital_Health;
	FGameplayTag Attributes_Vital_MaxMana;
	FGameplayTag Attributes_Vital_Mana;
	
	FGameplayTag Attributes_Primary_CriticalChance;
	FGameplayTag Attributes_Primary_CriticalDamage;
	FGameplayTag Attributes_Primary_Attack;
	FGameplayTag Attributes_Primary_Defense;
	FGameplayTag Attributes_Primary_DamageBonus;
	FGameplayTag Attributes_ElementalMastery;
	
	
	FGameplayTag Damage;
	FGameplayTag Resilience;
	FGameplayTag Effects_HitReact;
	
	// Buildable
	FGameplayTag Buildable_Door_Metal;
	FGameplayTag Buildable_Door_Stone;
	FGameplayTag Buildable_Door_Wood;
	
	FGameplayTag Buildable_Wall_Metal;
	FGameplayTag Buildable_Wall_Stone;
	FGameplayTag Buildable_Wall_Wood;
	
	FGameplayTag Event_OpenDoor;
	
	// Interactable
	FGameplayTag Interact;
	
	// Item Types
	FGameplayTag ItemType_Material;
	FGameplayTag ItemType_Consume;
	FGameplayTag ItemType_Building;
	FGameplayTag ItemType_Sell;
	FGameplayTag ItemType_Equipment;
	
	// Item Rarity
	FGameplayTag Rarity_Null;
	FGameplayTag Rarity_White;
	FGameplayTag Rarity_Green;
	FGameplayTag Rarity_Blue;
	FGameplayTag Rarity_Yellow;

private:
	
protected:
	static FSntpGameplayTags GameplayTags;
};