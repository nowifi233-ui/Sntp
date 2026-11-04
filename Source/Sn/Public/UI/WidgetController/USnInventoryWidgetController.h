// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnStorageComponent.h"
#include "UI/WidgetController/SnWidgetController.h"
#include "USnInventoryWidgetController.generated.h"

/**
 * Transient widget controller for every window whose data source is the
 * player's bag: inventory, chest (with storage injected), crafting, shop. The
 * bag windows are opened by ASnPlayerController (OpenCraftingWindow creates one
 * internally, the shop action calls CreateInventoryWidgetController and hands
 * it to OpenConfigWindow). Forwards the components' broadcasts so widgets bind
 * events to the controller and only reach for the component to READ
 * (GetInventory, GetGold, SellItem...). Fresh per window open (no stale state).
 */
UCLASS(BlueprintType)
class SN_API USnInventoryWidgetController : public USnWidgetController
{
	GENERATED_BODY()

public:
	/** The player's inventory + equipment component (main data source). */
	UFUNCTION(BlueprintPure, Category = "WidgetController")
	USnInventoryComponent* GetInventoryComponent() const;

	/** The chest/container being browsed (null for a plain bag window). */
	UFUNCTION(BlueprintPure, Category = "WidgetController")
	USnStorageComponent* GetStorage() const { return Storage; }

	/** Injects the chest storage when opening a container (open-chest flow). */
	void SetStorage(USnStorageComponent* InStorage);

	/** Forwarded from USnInventoryComponent (bag contents changed). */
	UPROPERTY(BlueprintAssignable, Category = "WidgetController")
	FSnInventoryChangedDelegate OnInventoryChanged;

	/** Forwarded from USnInventoryComponent (equipped slots changed). */
	UPROPERTY(BlueprintAssignable, Category = "WidgetController")
	FSnInventoryChangedDelegate OnEquipmentChanged;

	/** Forwarded from USnInventoryComponent (capacity changed - bag-expansion
	 *  items). Bag slots are rebuilt on this. */
	UPROPERTY(BlueprintAssignable, Category = "WidgetController")
	FSnCapacityChangedDelegate OnCapacityChanged;

	/** Forwarded from USnStorageComponent (container contents changed). */
	UPROPERTY(BlueprintAssignable, Category = "WidgetController")
	FSnStorageChangedDelegate OnStorageChanged;

	/** Forwarded from USnInventoryComponent (wallet changed). The server write
	 *  and the client's replicated OnRep both land here, so a window binds one
	 *  delegate regardless of net mode. */
	UPROPERTY(BlueprintAssignable, Category = "WidgetController")
	FSnGoldChangedDelegate OnGoldChanged;

protected:
	virtual void BindCallbacks() override;

	UFUNCTION()
	void HandleInventoryChanged();

	UFUNCTION()
	void HandleEquipmentChanged();

	UFUNCTION()
	void HandleCapacityChanged(int32 NewCapacity);

	UFUNCTION()
	void HandleStorageChanged();

	UFUNCTION()
	void HandleGoldChanged(int64 NewGold, int64 Delta);

private:
	UPROPERTY()
	TObjectPtr<USnStorageComponent> Storage;
};
