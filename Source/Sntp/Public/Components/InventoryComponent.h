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
	
	bool IsEmpty() const
	{
		return ItemDef == nullptr || Count <= 0; 
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, int32, Index);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInventoryComponent();

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Capacity = 35;
	
	UPROPERTY(BlueprintReadOnly)
	TArray<FItemInstance> Items;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryChanged OnInventoryChanged;
	
	void InitInventory();
	/*
	 * Operations
	 */
	
	int32 AddItem(UItemDefinition* ItemDef, int32 Count);
	bool RemoveItem(UItemDefinition* ItemDef, int32 Count);
	void UseItem(int32 Index);
	UFUNCTION(BlueprintCallable)
	void SwapItems(int32 A, int32 B);

private:
	int32 AddToExistingStacks(UItemDefinition* ItemDef, int32 Count);
	int32 AddToEmptySlots(UItemDefinition* ItemDef, int32 Count);
};
