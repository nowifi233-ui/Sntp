// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/InventoryWidgetController.h"

#include "UI/SntpHUD.h"

class ASntpHUD;

void UInventoryWidgetController::Init(UInventoryComponent* InInventoryComponent, APlayerController* InPlayerController)
{
	InventoryComponent = InInventoryComponent;
	if (InInventoryComponent)
	{
		InInventoryComponent->OnInventoryChanged.AddDynamic(this, &ThisClass::HandleInventoryChanged);
		PlayerController = InPlayerController;
	}
}

const TArray<FItemInstance>& UInventoryWidgetController::GetItems() const
{
	return InventoryComponent->Items;
}

void UInventoryWidgetController::RequestUseItem(int32 Index)
{
	if (InventoryComponent.Get())
	{
		InventoryComponent->UseItem(Index);
	}
}

void UInventoryWidgetController::RequestToggleBag()
{
	PlayerController->GetHUD<ASntpHUD>()->ToggleBag(PlayerController);
}

void UInventoryWidgetController::HandleInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}
