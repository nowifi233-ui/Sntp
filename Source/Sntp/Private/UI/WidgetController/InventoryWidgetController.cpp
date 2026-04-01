// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/InventoryWidgetController.h"

#include "UI/SntpHUD.h"

class ASntpHUD;

void UInventoryWidgetController::Init(UInventoryComponent* InInventoryComponent, APlayerController* InPlayerController)
{
	PlayerInventoryComponent = InInventoryComponent;
	if (InInventoryComponent)
	{
		InInventoryComponent->OnInventoryChanged.AddDynamic(this, &ThisClass::HandleInventoryChanged);
		PlayerController = InPlayerController;
	}
}

void UInventoryWidgetController::SetTargetInventoryComponent(UInventoryComponent* InInventoryComponent)
{
	TargetInventoryComponent = InInventoryComponent;
	if (InInventoryComponent)
	{
		InInventoryComponent->OnInventoryChanged.Clear();
		InInventoryComponent->OnInventoryChanged.AddDynamic(this, &ThisClass::HandleInventoryChanged);
	}
	EventTargetControllerSet.Broadcast();
}

const TArray<FItemInstance>& UInventoryWidgetController::GetItems() const
{
	return PlayerInventoryComponent->Items;
}

void UInventoryWidgetController::RequestUseItem(int32 Index)
{
	if (PlayerInventoryComponent.Get())
	{
		PlayerInventoryComponent->UseItem(Index);
	}
}

void UInventoryWidgetController::RequestToggleBag()
{
	PlayerController->GetHUD<ASntpHUD>()->ToggleBag(PlayerController);
}

void UInventoryWidgetController::RequestToggleInv()
{
	PlayerController->GetHUD<ASntpHUD>()->ToggleInventoryUI(PlayerController, nullptr);
}

void UInventoryWidgetController::TransferItem(UInventoryComponent* From, UInventoryComponent* To, int32 Index,
                                              int32 Count)
{
	UInventoryComponent::TransferItem(From, To, Index, Count);
}

void UInventoryWidgetController::HandleInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}
