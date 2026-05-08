#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"
#include "Data/ItemDefinition.h"
#include "FItemDropPair.generated.h"

USTRUCT(BlueprintType)
struct FItemDropPair
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	UItemDefinition* ItemDef = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	int32 Amount = 1;
	
	FItemDropPair() = default;
	
	FItemDropPair(UItemDefinition* InDef, int32 InAmount)
		: ItemDef(InDef), Amount(InAmount) {}
};