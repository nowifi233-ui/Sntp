// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"


struct FCraftingRecipe;
class UCraftingRecipeDataAsset;
class UInventoryComponent;
class UItemDefinition;



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCrafted, const FCraftingRecipe&, Recipe);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UCraftingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCraftingComponent();
	
	// injure Inventory Component
	void Init(UInventoryComponent* InInventory);
	
	UFUNCTION(BlueprintCallable)
	bool CanCraft(const FCraftingRecipe& Recipe) const;
	
	UFUNCTION(BlueprintCallable)
	int32 GetMaxCraftableCount(const FCraftingRecipe& Recipe) const;

	UFUNCTION(BlueprintCallable)
	int32 Craft(const FCraftingRecipe& Recipe);
	
	UFUNCTION(BlueprintCallable)
	int32 CraftMultiple(const FCraftingRecipe& Recipe, int32 Times);
public:	
	UPROPERTY(BlueprintAssignable)
	FOnCrafted OnCrafted;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UCraftingRecipeDataAsset> RecipeData;
	
	UFUNCTION()
	const TArray<FCraftingRecipe>& GetRecipes() const;
private:
	UPROPERTY()
	TObjectPtr<UInventoryComponent> Inventory;
	
	
};
