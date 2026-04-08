// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/CraftingWidgetController.h"
#include "Components/CraftingComponent/CraftingComponent.h"
#include "Components/CraftingComponent/CraftingTypes.h"
#include "UI/SntpHUD.h"

void UCraftingWidgetController::Init(UInventoryComponent* InInventory, UCraftingComponent* InCrafting, APlayerController* InPlayerController)
{
	InventoryComponent = InInventory;
	CraftingComponent = InCrafting;
	PlayerController = InPlayerController;
	InventoryComponent->OnInventoryChanged.AddDynamic(this, &UCraftingWidgetController::HandleOnInventoryChanged);
}

TArray<FCraftingRecipe> UCraftingWidgetController::GetRecipe()
{
	return CraftingComponent->GetRecipes();
}

void UCraftingWidgetController::ToggleCrafting()
{
	Cast<ASntpHUD>(PlayerController->GetHUD())->ToggleCraftingWidget(PlayerController);
}

void UCraftingWidgetController::Craft(const FCraftingRecipe& Recipe)
{
	CraftingComponent->Craft(Recipe);
}

void UCraftingWidgetController::CraftMultiple(const FCraftingRecipe& Recipe, int32 Count)
{
	CraftingComponent->CraftMultiple(Recipe, Count);
}

int32 UCraftingWidgetController::GetItemCount(UItemDefinition* Def)
{
	return InventoryComponent->GetItemCount(Def);
}

int32 UCraftingWidgetController::GetMaxCraftCount(const FCraftingRecipe& Recipe)
{
	return CraftingComponent->GetMaxCraftableCount(Recipe);
}

void UCraftingWidgetController::HandleOnInventoryChanged()
{
	OnInventoryChanged.Broadcast();
}





