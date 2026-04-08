// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "CraftingRecipeDataAsset.generated.h"

struct FCraftingRecipe;
/**
 * 
 */
UCLASS()
class SNTP_API UCraftingRecipeDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FCraftingRecipe> Recipes;
};
