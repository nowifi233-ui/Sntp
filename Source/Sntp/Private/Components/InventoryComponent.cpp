// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"

#include "ItemDefinition.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UInventoryComponent::InitInventory()
{
	Items.SetNum(Capacity);
}

int32 UInventoryComponent::AddItem(UItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <= 0) return Count;

	Count = AddToExistingStacks(ItemDef, Count);
	Count = AddToEmptySlots(ItemDef, Count);
	OnInventoryChanged.Broadcast();
	return Count;
}

bool UInventoryComponent::RemoveItem(UItemDefinition* ItemDef, int32 Count)
{
	// TODO: Finish it
	OnInventoryChanged.Broadcast();
	return false;
}

void UInventoryComponent::UseItem(int32 Index)
{
	if (!Items.IsValidIndex(Index)) return;
	FItemInstance& Item = Items[Index];
	if (Item.Count <= 0) return;
	// 调用物品逻辑
	Item.Count -= 1;
	if (Item.Count <= 0)
	{
		Items[Index] = FItemInstance();
	}
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::SwapItems(int32 A, int32 B)
{
	if (!Items.IsValidIndex(A) || !Items.IsValidIndex(B)) return;
	if (Items[A].ItemDef && Items[B].ItemDef)
	{
		if (Items[A].ItemDef->ItemID == Items[B].ItemDef->ItemID)
		{
			if (Items[B].Count < Items[B].ItemDef->MaxStack)
			{
				Items[B].Count = FMath::Min(Items[A].Count + Items[B].Count, Items[A].ItemDef->MaxStack);
				Items[A].Count = FMath::Max(0, Items[B].Count + Items[A].Count - Items[B].ItemDef->MaxStack);
				if (Items[A].Count == 0)
				{
					Items[A] = FItemInstance();
				}
			}
			OnInventoryChanged.Broadcast();
			return;
		}
	}
	Items.Swap(A, B);
	OnInventoryChanged.Broadcast();
}

int32 UInventoryComponent::AddToExistingStacks(UItemDefinition* ItemDef, int32 Count)
{
	for (FItemInstance& Item : Items)
	{
		if (Item.ItemDef == ItemDef && Item.Count < ItemDef->MaxStack)
		{
			int32 Space = ItemDef->MaxStack - Item.Count;
			int32 AddAmount = FMath::Min(Space, Count);
			Item.Count += AddAmount;
			Count -= AddAmount;
			if (Count <= 0)
			{
				return 0;
			}
		}
	}
	return Count;
}

int32 UInventoryComponent::AddToEmptySlots(UItemDefinition* ItemDef, int32 Count)
{
	for (FItemInstance& Item : Items)
	{
		if (Item.IsEmpty())
		{
			int32 AddAmount = FMath::Min(ItemDef->MaxStack, Count);
			Item.ItemDef = ItemDef;
			Item.Count = AddAmount;
			Count -= AddAmount;
			if (Count <= 0)
				return 0;
		}
	}
	return Count;
}



