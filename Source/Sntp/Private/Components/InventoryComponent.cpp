// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InventoryComponent.h"

#include "AbilitySystemGlobals.h"
#include "Data/ItemDefinition.h"
#include "Abilities/GameplayAbility.h"
#include "Characters/SntpPlayerCharacter.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UInventoryComponent::InitInventory()
{
	Items.Init(FItemInstance(), Capacity);
}

int32 UInventoryComponent::AddItem(UItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <= 0) return Count;

	OnItemAdded.Broadcast(ItemDef, Count);
	Count = AddToExistingStacks(ItemDef, Count);
	Count = AddToEmptySlots(ItemDef, Count);
	OnInventoryChanged.Broadcast();
	
	return Count;
}

bool UInventoryComponent::RemoveItem(UItemDefinition* ItemDef, int32 Count)
{
	if (!ItemDef || Count <= 0) return false;
	
	for (FItemInstance& Item : Items)
	{
		if (Item.ItemDef == ItemDef)
		{
			const int32 RemoveAmount = FMath::Min(Item.Count, Count);
			Item.Count -= RemoveAmount;
			Count -= RemoveAmount;
			if (Item.Count <= 0)
			{
				Item = FItemInstance();
			}
			if (Count <= 0)
			{
				OnInventoryChanged.Broadcast();
				return true;
			}
		}
	}
	OnInventoryChanged.Broadcast();
	return false;
}

void UInventoryComponent::UseItem(int32 Index)
{
	if (!Items.IsValidIndex(Index)) return;
	FItemInstance& Item = Items[Index];
	if (Item.Count <= 0) return;
	// 调用物品逻辑
	if (Item.ItemDef->UseAbility)
	{
		UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Item.ItemDef->UseAbility);
		ASC->GiveAbilityAndActivateOnce(AbilitySpec);
	}
	if (Item.ItemDef->UseEffect)
	{
		UAbilitySystemComponent* ASC = Cast<ASntpPlayerCharacter>(GetOwner())->GetAbilitySystemComponent();
		FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
		FGameplayEffectSpecHandle EffectSpecHandle = ASC->MakeOutgoingSpec(Item.ItemDef->UseEffect, 1, EffectContext);
		ASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data);
	}
	
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
	if (Items[A].ItemDef == nullptr)
	{
		return;
	}
	if (Items[A].ItemDef && Items[B].ItemDef)
	{
		if (Items[A].ItemDef->ItemID == Items[B].ItemDef->ItemID)
		{
			if (Items[B].Count < Items[B].ItemDef->MaxStack)
			{
				int32 TempB = Items[B].Count;
				Items[B].Count = FMath::Min(Items[A].Count + Items[B].Count, Items[A].ItemDef->MaxStack);
				Items[A].Count = FMath::Max(0, TempB + Items[A].Count - Items[B].ItemDef->MaxStack);
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

bool UInventoryComponent::HasItem(UItemDefinition* ItemDef, const int32 Count)
{
	if (!ItemDef || Count <= 0) return false;
	int32 Total = 0;
	for (const FItemInstance& Item  : Items)
	{
		if (Item.ItemDef == ItemDef)
		{
			Total += Item.Count;
			if (Total >= Count)
			{
				return true;
			}
		}
	}
	return false;
}

int32 UInventoryComponent::GetItemCount(UItemDefinition* ItemDef)
{
	int32 Count = 0;
	for (const FItemInstance& Item : Items)
	{
		if (Item.ItemDef == ItemDef)
		{
			Count += Item.Count;
		}
	}
	return Count;
}

int32 UInventoryComponent::FindItemSlot(UItemDefinition* ItemDef)
{
	if (!ItemDef) return false;
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		if (Items[i].ItemDef == ItemDef && Items[i].Count >= 1)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

bool UInventoryComponent::RemoveItemByIndex(int32 Index, int32 Count)
{
	if (!Items.IsValidIndex(Index)) return false;
	FItemInstance& Item = Items[Index];
	if (Item.Count < Count) return false;
	Item.Count -= Count;
	if (Item.Count <= 0)
	{
		Item = FItemInstance();
	}
	OnInventoryChanged.Broadcast();
	return true;
}

void UInventoryComponent::AutoSortItems()
{
	TArray<FItemInstance> AllItems;
	for (const FItemInstance& Item : Items)
	{
		if (!Item.IsEmpty())
		{
			AllItems.Add(Item);
		}
	}
	for (int32 i = 0; i < Items.Num(); ++i)
	{
		Items[i] = FItemInstance();
	}
	
	
	// Rebuild Items
	TMap<UItemDefinition*, int32> ItemCountMap;
	for (const FItemInstance& Item : AllItems)
	{
		ItemCountMap.FindOrAdd(Item.ItemDef) += Item.Count;
	}
	int32 Index = 0;
	for (auto& Pair : ItemCountMap)
	{
		UItemDefinition* Def = Pair.Key;
		int32 TotalCount = Pair.Value;
		while (TotalCount > 0 && Index < Items.Num())
		{
			int32 StackSize = FMath::Min(Def->MaxStack, TotalCount);
			Items[Index] = FItemInstance(Def, StackSize);
			TotalCount -= StackSize;
			Index++;
		}
	}
	SortItems(Items);
	OnInventoryChanged.Broadcast();
}

void UInventoryComponent::TransferItem(UInventoryComponent* From, UInventoryComponent* To, int32 Index, int32 Count)
{
	if (!From || !To) return;
	FItemInstance& Item = From->Items[Index];
	int32 Remaining = To->AddItem(Item.ItemDef, Count);
	int32 Removed = Count - Remaining;
	From->RemoveItemByIndex(Index, Removed);
}

bool UInventoryComponent::SwapItemsByIndex(UInventoryComponent* From, UInventoryComponent* To, int32 FromIndex,
	int32 ToIndex)
{
	
	if (!From || !To) return false;
	if (!From->Items.IsValidIndex(FromIndex) || !To->Items.IsValidIndex(ToIndex)) return false;
	if (From->Items[FromIndex].ItemDef == nullptr)
	{
		return false;
	}
	FItemInstance& FromItem = From->Items[FromIndex];
	FItemInstance& ToItem = To->Items[ToIndex];
	
	if (FromItem.IsEmpty()) return false;
	
	/**
	 * Situaction1: if To is empty
	 */
	if (ToItem.IsEmpty())
	{
		ToItem = FromItem;
		FromItem = FItemInstance();
		if (From == To)
		{
			From->OnInventoryChanged.Broadcast();
		}
		else
		{
			From->OnInventoryChanged.Broadcast();
			To->OnInventoryChanged.Broadcast();
		}
		return true;
	}
	/**
	 * Situation2: if Same Item Def
	 */
	if (FromItem.ItemDef == ToItem.ItemDef)
	{
		int32 MaxStack = FromItem.ItemDef->MaxStack;
		int32 Space = MaxStack - ToItem.Count;
		if (Space > 0)
		{
			int32 MoveCount = FMath::Min(Space, FromItem.Count);
			ToItem.Count += MoveCount;
			FromItem.Count -= MoveCount;
			if (FromItem.Count <= 0)
			{
				FromItem = FItemInstance();
			}
			if (From == To)
			{
				From->OnInventoryChanged.Broadcast();
			}
			else
			{
				From->OnInventoryChanged.Broadcast();
				To->OnInventoryChanged.Broadcast();
			}
			return true;
		}
	}
	/**
	 * Situation3: Different Item def
	 */
	Swap(FromItem, ToItem);
	if (From == To)
	{
		From->OnInventoryChanged.Broadcast();
	}
	else
	{
		From->OnInventoryChanged.Broadcast();
		To->OnInventoryChanged.Broadcast();
	}
	return true;
}

bool UInventoryComponent::QuickMoveItems(UInventoryComponent* From, UInventoryComponent* To, int32 FromIndex)
{
	if (!From || !To) return false;
	if (!From->Items.IsValidIndex(FromIndex)) return false;
	if (From->Items[FromIndex].ItemDef == nullptr)
	{
		return false;
	}
	int32 AddAmount = From->Items[FromIndex].Count;
	int32 Remain = AddToExistingStacks(From->Items[FromIndex].ItemDef, AddAmount, To);
	From->RemoveItemByIndex(FromIndex, AddAmount - Remain);
	AddAmount = Remain;
	
	if (Remain <= 0) return true;
	Remain = AddToEmptySlots(From->Items[FromIndex].ItemDef, AddAmount, To);
	From->RemoveItemByIndex(FromIndex, AddAmount - Remain);
	AddAmount = Remain;
	
	if (AddAmount <= 0) return true;
	return false;
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
	if (Count <= 0) return 0;
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

int32 UInventoryComponent::AddToExistingStacks(UItemDefinition* ItemDef, int32 Count, UInventoryComponent* To)
{
	for (FItemInstance& Item : To->Items)
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

int32 UInventoryComponent::AddToEmptySlots(UItemDefinition* ItemDef, int32 Count, UInventoryComponent* To)
{
	if (Count <= 0) return 0;
	for (FItemInstance& Item : To->Items)
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

void UInventoryComponent::SortItems(TArray<FItemInstance>& InputItems)
{
	InputItems.Sort([](const FItemInstance& A, const FItemInstance& B)
	{
		if (A.IsEmpty()) return false;
		if (B.IsEmpty()) return true;
		// Type
		if (A.ItemDef->ItemType != B.ItemDef->ItemType)
		{
			return A.ItemDef->ItemType < B.ItemDef->ItemType;
		}
		if (A.ItemDef->ItemID != B.ItemDef->ItemID)
		{
			return A.ItemDef->ItemID.ToUnstableInt() < B.ItemDef->ItemID.ToUnstableInt();
		}
		return A.Count > B.Count;
	});
}



