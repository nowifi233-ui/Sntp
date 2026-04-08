
#pragma once

#include "CoreMinimal.h"
#include "CraftingTypes.generated.h"

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