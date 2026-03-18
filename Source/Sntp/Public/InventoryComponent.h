// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType, meta=(ShowInnerProperties))
struct FItemInstance
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UItemDefinition> ItemDef;
	
	UPROPERTY(BlueprintReadOnly)
	int32 Count = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	UPROPERTY(BlueprintReadOnly)
	TArray<FItemInstance> Items;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChanged;
	
	bool AddItem(UItemDefinition* ItemDef, int32 Count);
	bool RemoveItem(UItemDefinition* ItemDef, int32 Count);
		
};
