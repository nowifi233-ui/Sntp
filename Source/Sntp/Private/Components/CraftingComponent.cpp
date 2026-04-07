// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/CraftingComponent.h"

#include "Components/InventoryComponent.h"

// Sets default values for this component's properties
UCraftingComponent::UCraftingComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UCraftingComponent::Init(UInventoryComponent* InInventory)
{
	Inventory = InInventory;
}

bool UCraftingComponent::CanCraft(const FCraftingRecipe& Recipe) const
{
	if (!Inventory) return false;
	for (const auto& Pair : Recipe.Inputs)
	{
		UItemDefinition* ItemDef = Pair.Key;
		int32 Required  = Pair.Value;
		
		if (!Inventory->HasItem(ItemDef, Required))
		{
			return false;
		}
	}
	return true;
}



int32 UCraftingComponent::GetMaxCraftableCount(const FCraftingRecipe& Recipe) const
{
	if (!Inventory) return 0;
	int32 MaxCount = INT32_MAX;
	for (const auto& Pair : Recipe.Inputs)
	{
		UItemDefinition* ItemDef = Pair.Key;
		int32 Required  = Pair.Value;
		int32 Owned = Inventory->GetItemCount(ItemDef);
		int32 Craftable = Owned / Required;
		MaxCount = FMath::Min(MaxCount, Craftable);
	}
	return MaxCount == INT32_MAX ? 0 : MaxCount;
}

int32 UCraftingComponent::Craft(const FCraftingRecipe& Recipe)
{
	if (!CanCraft(Recipe)) return false;
	for (const auto& Pair : Recipe.Inputs)
	{
		Inventory->RemoveItem(Pair.Key, Pair.Value);
	}
	Inventory->AddItem(Recipe.Output, Recipe.OutputCount);
	OnCrafted.Broadcast(Recipe);
	return true;
}

int32 UCraftingComponent::CraftMultiple(const FCraftingRecipe& Recipe, int32 Times)
{
	if (!Inventory || Times <= 0) return 0;
	
	int32 MaxCraft = GetMaxCraftableCount(Recipe);
	int32 ActualTimes = FMath::Min(MaxCraft, Times);
	
	if (ActualTimes <= 0) return 0;
	
	for (auto& Pair : Recipe.Inputs)
	{
		Inventory->RemoveItem(Pair.Key, Pair.Value * ActualTimes);
	}
	Inventory->AddItem(Recipe.Output, Recipe.OutputCount * ActualTimes);
	return ActualTimes;
}











