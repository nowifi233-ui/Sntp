// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SntpHUD.h"

#include "Characters/SntpPlayerCharacter.h"
#include "UI/WidgetController/OverlayWidgetController.h"

UOverlayWidgetController* ASntpHUD::GetOverlayWidgetController(const FWidgetControllerParams& Params)
{
	if (OverlayWidgetController == nullptr)
	{
		OverlayWidgetController = NewObject<UOverlayWidgetController>(this, OverlayWidgetControllerClass);
		OverlayWidgetController->SetWidgetControllerParams(Params);
	}
	return OverlayWidgetController;
}

UOverlayWidgetController* ASntpHUD::GetOverlayWidgetControllerWithoutParam()
{
	return OverlayWidgetController;
}

void ASntpHUD::InitOverlay(APlayerController* PlayerController, APlayerState* PlayerState, UAbilitySystemComponent* AbilitySystemComponent, UAttributeSet* AttributeSet)
{
	checkf(OverlayWidgetClass, TEXT("OverlayWidgetClass uninitialized, please check SntpHUD."))
	checkf(OverlayWidgetControllerClass, TEXT("OverlayWidgetControllerClass uninitialized, please check SntpHUD"))
	
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	OverlayWidget = Cast<USntpUserWidget>(Widget);
	
	const FWidgetControllerParams WidgetControllerParams(PlayerController, PlayerState, AbilitySystemComponent, AttributeSet);
	OverlayWidgetController = GetOverlayWidgetController(WidgetControllerParams);
	OverlayWidgetController->BindCallbackToDependencies();
	OverlayWidget->SetWidgetController(OverlayWidgetController);
	
	OverlayWidgetController->BroadcastInitialValue();
	Widget->AddToViewport();
	
	// Init Bag
	if (!InventoryWidgetController)
	{
		InventoryWidgetController = NewObject<UInventoryWidgetController>(this, InventoryWidgetControllerClass);
		InventoryWidgetController->Init(PlayerController->GetPawn<ASntpPlayerCharacter>()->InventoryComponent, GetOwningPlayerController());
	}
}

void ASntpHUD::ToggleBag(APlayerController* PlayerController)
{
	if (!PlayerController) return;
	if (!BagWidget)
	{
		UUserWidget* Widget  = CreateWidget<UUserWidget>(GetWorld(), BagWidgetClass);
		BagWidget = Cast<USntpUserWidget>(Widget);
		
		if (!InventoryWidgetController)
		{
			InventoryWidgetController = NewObject<UInventoryWidgetController>(this, InventoryWidgetControllerClass);
			InventoryWidgetController->Init(PlayerController->GetPawn<ASntpPlayerCharacter>()->InventoryComponent, GetOwningPlayerController());
		}
		
		BagWidget->SetWidgetController(InventoryWidgetController);
	}
	if (bBagOpen)
	{
		BagWidget->RemoveFromParent();
		// PlayerController->SetInputMode(FInputModeGameOnly());
		bBagOpen = false;
	}
	else
	{
		BagWidget->AddToViewport();
		bBagOpen = true;
	}
}

