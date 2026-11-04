// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/FSnItemTypes.h"
#include "USnStorageComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSnStorageChangedDelegate);

class USnInventoryComponent;

/**
 * A world container (chest, stash): its own replicated stack array with a
 * capacity. Mounted on chest actors next to their UInteractableComponent.
 * Transfers go through the Server* RPCs so the bag and the container stay
 * consistent on the server; each side rolls back when the target is full.
 */
UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class SN_API USnStorageComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USnStorageComponent();

	/** Adds to the container (server-side; used by loot/placement logic). */
	int32 AddItem(FName ItemRowName, int32 Count);

	/** Removes from the container (server-side). */
	bool RemoveItem(FName ItemRowName, int32 Count);

	/** Bag -> container (client-safe; routes through the server RPC). SlotIndex
	 *  is the clicked bag slot (optional; -1 = INDEX_NONE = row-name path). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Storage")
	void TransferToStorage(FName ItemRowName, int32 Count, USnInventoryComponent* Inventory, int32 SlotIndex = -1) { ServerTransferToStorage(ItemRowName, Count, Inventory, SlotIndex); }

	/** Container -> bag (client-safe; routes through the server RPC). SlotIndex
	 *  is the clicked container slot (optional; -1 = INDEX_NONE = row-name path). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Storage")
	void TransferToInventory(FName ItemRowName, int32 Count, USnInventoryComponent* Inventory, int32 SlotIndex = -1) { ServerTransferToInventory(ItemRowName, Count, Inventory, SlotIndex); }

	/** Swaps/merges two slots within the container (drag-drop; server-executed).
	 *  Same row name merges into the target; any slot (including empty ones)
	 *  can be a drop target - items swap freely, so dropping onto an empty
	 *  slot places the item there. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Storage")
	void SwapStacks(int32 FromIndex, int32 ToIndex) { ServerSwapStacks(FromIndex, ToIndex); }

	/** Removes Count of ItemRowName from the clicked slot (SlotIndex) when the
	 *  slot still holds it; falls back to the row-wide removal otherwise (same
	 *  semantics as USnInventoryComponent::RemoveItemAt). Broadcasts
	 *  OnStorageChanged on success. */
	bool RemoveItemAt(FName ItemRowName, int32 SlotIndex, int32 Count);

	/** One-click sort: merges stacks, re-packs in row-name order (server). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Storage")
	void SortStorage() { ServerSortStorage(); }

	// --- Queries (UI) ---

	UFUNCTION(BlueprintPure, Category = "Sn|Storage")
	const TArray<FSnItemStack>& GetContents() const { return Contents; }

	UFUNCTION(BlueprintPure, Category = "Sn|Storage")
	int32 GetItemCount(FName ItemRowName) const;

	UFUNCTION(BlueprintPure, Category = "Sn|Storage")
	int32 GetCapacity() const { return Capacity; }

	/** True when every slot holds an item (no empty slot to drop into). */
	UFUNCTION(BlueprintPure, Category = "Sn|Storage")
	bool IsFull() const;

	/** Max number of slots; set per-instance in the editor/BP defaults. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Storage")
	int32 Capacity = 20;

	/** Fired whenever the contents change (also on replication). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Storage")
	FSnStorageChangedDelegate OnStorageChanged;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerTransferToStorage(FName ItemRowName, int32 Count, USnInventoryComponent* Inventory, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerTransferToInventory(FName ItemRowName, int32 Count, USnInventoryComponent* Inventory, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerSwapStacks(int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable)
	void ServerSortStorage();

	UFUNCTION()
	void OnRep_Contents();

	/** Keeps the slot array length in sync with the capacity (pads empty slots). */
	void ResizeToCapacity();

	UPROPERTY(ReplicatedUsing = OnRep_Contents)
	TArray<FSnItemStack> Contents;
};
