// Fill out your copyright notice in the Description page of Project Settings.


#include "SntpGameplayTags.h"

#include "AssetTypeCategories.h"
#include "GameplayTagsManager.h"

FSntpGameplayTags FSntpGameplayTags::GameplayTags;

void FSntpGameplayTags::InitializeNativeGameplayTags()
{
	/**
	 * Vital Attributes
	 * MaxHealth, Health, MaxMana, Mana;
	 */
	
	 GameplayTags.Attributes_Vital_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxHealth"),
		FString("MaxHealth"));
	
	GameplayTags.Attributes_Vital_Health = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Health"),
		FString("Health"));
	
	GameplayTags.Attributes_Vital_MaxMana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxMana"),
		FString("Mana"));
	
	GameplayTags.Attributes_Vital_Mana = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.Mana"),
		FString("Mana"));
	
	GameplayTags.Damage = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Damage"),
		FString("Damage"));
	
	GameplayTags.Resilience = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Resilience"),
		FString("Resilience"));
	
	/*
	 * Effects grant
	 */
	GameplayTags.Effects_HitReact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Effects.HitReact"),
		FString("Tag granted when reacting."));
	
	/*
	 * Interactable
	 */
	GameplayTags.Interact = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Interact"),
		FString("Interact"));
	
	
	/**
	 * Buildable
	 */
	
	GameplayTags.Buildable_Door_Metal = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Buildable.Door.Metal"),
	   FString("Buildable"));
	
	GameplayTags.Buildable_Door_Stone = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Buildable.Door.Stone"),
	   FString("Buildable"));
	
	GameplayTags.Buildable_Door_Wood = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Buildable.Door.Wood"),
	   FString("Buildable"));
	
	GameplayTags.Buildable_Wall_Metal = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Buildable.Wall.Metal"),
	   FString("Buildable"));
	
	GameplayTags.Buildable_Wall_Stone = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Buildable.Wall.Stone"),
	   FString("Buildable"));
	
	GameplayTags.Buildable_Wall_Wood = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Buildable.Wall.Wood"),
	   FString("Buildable"));
	
	GameplayTags.Event_OpenDoor = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("Event.OpenDoor"),
	   FString("Buildable"));
	
	/**
	 * Item Types
	 */
	
	GameplayTags.ItemType_Material = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("ItemType.Material"),
	   FString("Material"));
	
		
	GameplayTags.ItemType_Consume = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("ItemType.Consume"),
	   FString("Consume"));
	
		
	GameplayTags.ItemType_Building = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("ItemType.Building"),
	   FString("Building"));
	
		
	GameplayTags.ItemType_Sell = UGameplayTagsManager::Get().AddNativeGameplayTag(
	   FName("ItemType.Sell"),
	   FString("Sell"));
}



