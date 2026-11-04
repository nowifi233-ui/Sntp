// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Inventory/FSnItemTypes.h"
#include "UObject/NoExportTypes.h"
#include "UItemAction.generated.h"

class USnInventoryComponent;
class USnStorageComponent;

/**
 * A single item action (right-click context menu entry). Subclass in
 * blueprint (BP_Action_UseItem etc.) and override Execute; the action row
 * (FSnItemActionRow::ActionClass) decides which subclass runs, so neither
 * C++ nor the menu switches on action type. Stateless and instantiated per
 * execution (same pattern as UInteractionAction), fully reusable.
 */
UCLASS(Blueprintable, Abstract)
class SN_API UItemAction : public UObject
{
	GENERATED_BODY()

public:
	/** Runs the action against the clicked item. Override in a blueprint
	 *  subclass; the inventory/storage components are injected by the caller
	 *  (the bag lives on the PlayerState, the storage on a chest actor - the
	 *  action doesn't hunt for either). */
	UFUNCTION(BlueprintNativeEvent, Category = "ItemAction")
	void Execute(USnInventoryComponent* Inventory, USnStorageComponent* Storage, const FSnItemActionContext& Context);
	virtual void Execute_Implementation(USnInventoryComponent* Inventory, USnStorageComponent* Storage, const FSnItemActionContext& Context);
};
