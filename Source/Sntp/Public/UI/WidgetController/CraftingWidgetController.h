// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InventoryWidgetController.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "CraftingWidgetController.generated.h"

struct FCraftingRecipe;
class UCraftingComponent;
class UInventoryComponent;
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SNTP_API UCraftingWidgetController : public USntpWidgetController
{
	GENERATED_BODY()
	
public:
	void Init(UInventoryComponent* InInventory, UCraftingComponent* InCrafting, APlayerController* InPlayerController);
	
	UFUNCTION(BlueprintCallable)
	TArray<FCraftingRecipe> GetRecipe();
	
	UFUNCTION(BlueprintCallable)
	void ToggleCrafting();
	
	UFUNCTION(BlueprintCallable)
	void Craft(const FCraftingRecipe& Recipe);
	
	UFUNCTION(BlueprintCallable)
	void CraftMultiple(const FCraftingRecipe& Recipe, int32 Count);
	
	UPROPERTY(BlueprintAssignable)
	FInventoryChangedDelegate OnInventoryChanged;
	
	UFUNCTION(BlueprintCallable)
	int32 GetItemCount(UItemDefinition* Def);
	
	UFUNCTION(BlueprintCallable)
	int32 GetMaxCraftCount(const FCraftingRecipe& Recipe);
	
	
private:
	UPROPERTY()
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
	UPROPERTY()
	TObjectPtr<UCraftingComponent> CraftingComponent;
	
	UFUNCTION()
	void HandleOnInventoryChanged();
};
