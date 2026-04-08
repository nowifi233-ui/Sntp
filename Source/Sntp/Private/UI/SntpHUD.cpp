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
	ToggleMouse();
}

void ASntpHUD::ToggleInventoryUI(APlayerController* PlayerController, UInventoryComponent* TargetInventoryComponent)
{
	if (!PlayerController) return;
	if (!InventoryWidget)
	{
		UUserWidget* Widget  = CreateWidget<UUserWidget>(GetWorld(), InventoryWidgetClass);
		InventoryWidget = Cast<USntpUserWidget>(Widget);
		
		if (!InventoryWidgetController)
		{
			InventoryWidgetController = NewObject<UInventoryWidgetController>(this, InventoryWidgetControllerClass);
			InventoryWidgetController->Init(PlayerController->GetPawn<ASntpPlayerCharacter>()->InventoryComponent, GetOwningPlayerController());
		}
		InventoryWidgetController->SetTargetInventoryComponent(TargetInventoryComponent);
		InventoryWidget->SetWidgetController(InventoryWidgetController);
	}
	
	if (bInventoryOpen)
	{
		InventoryWidget->RemoveFromParent();
		// PlayerController->SetInputMode(FInputModeGameOnly());
		bInventoryOpen = false;
	}
	else
	{
		InventoryWidgetController->SetTargetInventoryComponent(TargetInventoryComponent);
		InventoryWidget->AddToViewport();
		bInventoryOpen = true;
	}
	ToggleMouse();
}

void ASntpHUD::ToggleSettingMenu(APlayerController* PlayerController)
{
	if (!SettingWidget)
	{
		SettingWidget = CreateWidget<UUserWidget>(GetWorld(), SettingWidgetClass);
	}
	if (!bSettingOpen)
	{
		
		SettingWidget->AddToViewport();
		bSettingOpen = true;
	}
	else
	{
		bSettingOpen = false;
		SettingWidget->RemoveFromParent();
	}
	ToggleMouse();
}

void ASntpHUD::ToggleCraftingWidget(APlayerController* PlayerController)
{
	if (!PlayerController) return;
	if (!CraftingWidget)
	{
		UUserWidget* Widget  = CreateWidget<UUserWidget>(GetWorld(), CraftingWidgetClass);
		CraftingWidget = Cast<USntpUserWidget>(Widget);
		
		if (!CraftingWidgetController)
		{
			CraftingWidgetController = NewObject<UCraftingWidgetController>(this, CraftingWidgetControllerClass);
			CraftingWidgetController->Init(PlayerController->GetPawn<ASntpPlayerCharacter>()->InventoryComponent, PlayerController->GetPawn<ASntpPlayerCharacter>()->CraftingComponent, PlayerController);
		}
		CraftingWidget->SetWidgetController(CraftingWidgetController);
	}
	
	if (bCraftingOpen)
	{
		CraftingWidget->RemoveFromParent();
		// PlayerController->SetInputMode(FInputModeGameOnly());
		bCraftingOpen = false;
	}
	else
	{
		CraftingWidget->AddToViewport();
		bCraftingOpen = true;
	}
	ToggleMouse();
}

bool ASntpHUD::ShouldHideMouse() const
{
	if (bSettingOpen || bBagOpen || bInventoryOpen || bCraftingOpen)
	{
		return false;
	}
	return true;
}

void ASntpHUD::ToggleMouse()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	
	if (!PlayerController)
	{
		return;
	}
	if (ShouldHideMouse())
	{
		PlayerController->bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetIgnoreLookInput(false);
		PlayerController->SetIgnoreMoveInput(false);
	}
	else
	{
		PlayerController->bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(false);
		PlayerController->SetInputMode(InputMode);
	}
}

