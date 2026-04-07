// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CraftingComponent.generated.h"


class UInventoryComponent;
class UItemDefinition;

USTRUCT(BlueprintType)
struct FCraftingRecipe
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TMap<TObjectPtr<UItemDefinition>, int32> Inputs;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UItemDefinition> Output;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 OutputCount = 1;
};

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCraftingRecipe> Recipes;
	
private:
	UPROPERTY()
	TObjectPtr<UInventoryComponent> Inventory;
};
