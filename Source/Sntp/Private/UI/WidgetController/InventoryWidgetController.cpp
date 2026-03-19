// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/InventoryWidgetController.h"

void UInventoryWidgetController::Init(UInventoryComponent* InInventoryComponent)
{
	InventoryComponent = InInventoryComponent;
	if (InInventoryComponent)
	{
		InInventoryComponent->OnInventoryChanged.AddDynamic(this, &ThisClass::HandleInventoryChanged);
	}
}

const TArray<FItemInstance>& UInventoryWidgetController::GetItems() const
{
	return InventoryComponent->Items;
}

void UInventoryWidgetController::HandleInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}
