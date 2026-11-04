// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/USnStorageComponent.h"

#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnItemLibrary.h"
#include "Net/UnrealNetwork.h"

USnStorageComponent::USnStorageComponent()
{
	SetIsReplicatedByDefault(true);
}

void USnStorageComponent::BeginPlay()
{
	Super::BeginPlay();
	ResizeToCapacity();
}

void USnStorageComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USnStorageComponent, Contents);
}

int32 USnStorageComponent::AddItem(FName ItemRowName, int32 Count)
{
	ResizeToCapacity();
	const int32 AddedTotal = USnItemLibrary::AddItemToStacks(Contents, ItemRowName, Count);
	if (AddedTotal > 0)
	{
		OnStorageChanged.Broadcast();
	}
	return AddedTotal;
}

bool USnStorageComponent::RemoveItem(FName ItemRowName, int32 Count)
{
	const bool bRemoved = USnItemLibrary::RemoveItemFromStacks(Contents, ItemRowName, Count);
	if (bRemoved)
	{
		OnStorageChanged.Broadcast();
	}
	return bRemoved;
}

bool USnStorageComponent::RemoveItemAt(FName ItemRowName, int32 SlotIndex, int32 Count)
{
	// Slot-exact removal when the clicked slot still holds the item; row-name
	// fallback otherwise (mirrors USnInventoryComponent::RemoveItemAt).
	if (SlotIndex != INDEX_NONE && Contents.IsValidIndex(SlotIndex)
		&& Contents[SlotIndex].ItemRowName == ItemRowName)
	{
		if (USnItemLibrary::RemoveItemFromStackAt(Contents, SlotIndex, Count))
		{
			OnStorageChanged.Broadcast();
			return true;
		}
	}
	return RemoveItem(ItemRowName, Count);
}

void USnStorageComponent::ResizeToCapacity()
{
	if (Contents.Num() < Capacity)
	{
		// Pad with empty slots (default-constructed = ItemRowName None).
		Contents.SetNum(Capacity);
	}
}

// --- Transfers (server only) ---

void USnStorageComponent::ServerTransferToStorage_Implementation(FName ItemRowName, int32 Count, USnInventoryComponent* Inventory, int32 SlotIndex)
{
	if (!Inventory || Count <= 0)
	{
		return;
	}

	// Bag first (from the clicked slot, row-name fallback inside), then
	// container; put back whatever didn't fit (partial-add rollback).
	if (!Inventory->RemoveItemAt(ItemRowName, SlotIndex, Count))
	{
		return;
	}
	const int32 Added = AddItem(ItemRowName, Count);
	if (Added < Count)
	{
		Inventory->AddItem(ItemRowName, Count - Added);
	}
}

void USnStorageComponent::ServerTransferToInventory_Implementation(FName ItemRowName, int32 Count, USnInventoryComponent* Inventory, int32 SlotIndex)
{
	if (!Inventory || Count <= 0)
	{
		return;
	}

	if (!RemoveItemAt(ItemRowName, SlotIndex, Count))
	{
		return;
	}
	const int32 Added = Inventory->AddItem(ItemRowName, Count);
	if (Added < Count)
	{
		AddItem(ItemRowName, Count - Added);
	}
}

void USnStorageComponent::ServerSwapStacks_Implementation(int32 FromIndex, int32 ToIndex)
{
	ResizeToCapacity();
	if (FromIndex == ToIndex || !Contents.IsValidIndex(FromIndex) || !Contents.IsValidIndex(ToIndex))
	{
		return;
	}

	FSnItemStack& Source = Contents[FromIndex];
	FSnItemStack& Target = Contents[ToIndex];
	if (!Source.ItemRowName.IsNone() && !Target.ItemRowName.IsNone()
		&& Source.ItemRowName == Target.ItemRowName)
	{
		// Merge into the target, capped at the stack size. Overflow stays in
		// the source slot, which empties out when fully merged.
		const int32 StackSize = USnItemLibrary::GetItemStackSize(Target.ItemRowName);
		const int32 Room = StackSize - Target.Count;
		const int32 Merged = FMath::Min(Source.Count, Room);
		Target.Count += Merged;
		Source.Count -= Merged;
		if (Source.Count <= 0)
		{
			Source.ItemRowName = NAME_None;
		}
	}
	else
	{
		// Fixed-slot model: swap the slot contents outright - empty slots swap
		// freely, so dropping onto an empty slot places the item there.
		Swap(Contents[FromIndex], Contents[ToIndex]);
	}

	OnStorageChanged.Broadcast();
}

void USnStorageComponent::ServerSortStorage_Implementation()
{
	ResizeToCapacity();
	USnItemLibrary::SortStacks(Contents);
	OnStorageChanged.Broadcast();
}

// --- Queries ---

bool USnStorageComponent::IsFull() const
{
	for (const FSnItemStack& Stack : Contents)
	{
		if (Stack.ItemRowName.IsNone())
		{
			return false;
		}
	}
	return true;
}

int32 USnStorageComponent::GetItemCount(FName ItemRowName) const
{
	return USnItemLibrary::CountItemInStacks(Contents, ItemRowName);
}

// --- Replication callback ---

void USnStorageComponent::OnRep_Contents()
{
	OnStorageChanged.Broadcast();
}
