// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/InventoryComponent.h"
#include "ItemDragDropOperation.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable, meta=( DontUseGenericSpawnObject="True" ))
class SNTP_API UItemDragDropOperation : public UDragDropOperation
{
	GENERATED_BODY()
public:
	UPROPERTY(BlueprintReadWrite)
	int32 FromIndex;
	
	UPROPERTY(BlueprintReadWrite)
	UInventoryComponent* SourceInventory;
};
