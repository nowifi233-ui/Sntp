// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

#include "ItemDefinition.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

bool UInventoryComponent::AddItem(UItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef)
	{
		return false;
	}
	if (ItemDef->bStackable)
	{
		for (auto& Item : Items)
		{
			if (Item.ItemDef == ItemDef)
			{
				Item.Count += Count;
				OnInventoryChanged.Broadcast();
				return true;
			}
		}
	}
	FItemInstance NewItem;
	NewItem.ItemDef = ItemDef;
	NewItem.Count = Count;
	Items.Add(NewItem);
	OnInventoryChanged.Broadcast();
	return true;
}

bool UInventoryComponent::RemoveItem(UItemDefinition* ItemDef, int32 Count)
{
	// TODO: Finish it
	OnInventoryChanged.Broadcast();
	return false;
}



