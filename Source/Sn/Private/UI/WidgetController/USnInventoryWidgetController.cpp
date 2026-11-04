// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/USnInventoryWidgetController.h"

#include "Player/SnPlayerState.h"

USnInventoryComponent* USnInventoryWidgetController::GetInventoryComponent() const
{
	if (const ASnPlayerState* SnPlayerState = Cast<ASnPlayerState>(PlayerState))
	{
		return SnPlayerState->GetInventoryComponent();
	}
	return nullptr;
}

void USnInventoryWidgetController::SetStorage(USnStorageComponent* InStorage)
{
	if (Storage)
	{
		Storage->OnStorageChanged.RemoveDynamic(this, &USnInventoryWidgetController::HandleStorageChanged);
	}
	Storage = InStorage;
	if (Storage)
	{
		Storage->OnStorageChanged.AddDynamic(this, &USnInventoryWidgetController::HandleStorageChanged);
	}
}

void USnInventoryWidgetController::BindCallbacks()
{
	if (USnInventoryComponent* Inventory = GetInventoryComponent())
	{
		Inventory->OnInventoryChanged.AddDynamic(this, &USnInventoryWidgetController::HandleInventoryChanged);
		Inventory->OnEquipmentChanged.AddDynamic(this, &USnInventoryWidgetController::HandleEquipmentChanged);
		Inventory->OnCapacityChanged.AddDynamic(this, &USnInventoryWidgetController::HandleCapacityChanged);
		Inventory->OnGoldChanged.AddDynamic(this, &USnInventoryWidgetController::HandleGoldChanged);
	}
	if (Storage)
	{
		Storage->OnStorageChanged.AddDynamic(this, &USnInventoryWidgetController::HandleStorageChanged);
	}
}

void USnInventoryWidgetController::HandleInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}

void USnInventoryWidgetController::HandleEquipmentChanged()
{
	OnEquipmentChanged.Broadcast();
}

void USnInventoryWidgetController::HandleCapacityChanged(int32 NewCapacity)
{
	OnCapacityChanged.Broadcast(NewCapacity);
}

void USnInventoryWidgetController::HandleStorageChanged()
{
	OnStorageChanged.Broadcast();
}

void USnInventoryWidgetController::HandleGoldChanged(int64 NewGold, int64 Delta)
{
	OnGoldChanged.Broadcast(NewGold, Delta);
}
