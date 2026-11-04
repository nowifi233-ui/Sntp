// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/UItemAction.h"

void UItemAction::Execute_Implementation(USnInventoryComponent* Inventory, USnStorageComponent* Storage, const FSnItemActionContext& Context)
{
	// Base implementation is intentionally a warning: subclasses must override.
	UE_LOG(LogTemp, Warning, TEXT("UItemAction: %s has no Execute override"), *GetName());
}
