// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "Components/ActorComponent.h"
#include "Components/InventoryComponent.h"
#include "Data/ItemDefinition.h"
#include "EquipmentComponent.generated.h"


class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentChanged);

USTRUCT(BlueprintType)
struct FEquipmentSlotData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EEquipmentSlot Slot = EEquipmentSlot::None;

	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UItemDefinition> ItemDef = nullptr;
	
	FActiveGameplayEffectHandle EffectHandle;

	bool IsEmpty() const
	{
		return ItemDef == nullptr;
	}
};

UCLASS(Blueprintable, BlueprintType/*, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent)*/)
class SNTP_API UEquipmentComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEquipmentComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:

	void InitializeSlots();

public:

	UPROPERTY(BlueprintReadOnly)
	TArray<FEquipmentSlotData> EquipmentSlots;

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
	FOnEquipmentChanged OnEquipmentChanged;
	
	UFUNCTION(BlueprintCallable)
	void BroadCastOnEquipmentChanged();

public:

	UFUNCTION(BlueprintCallable)
	bool EquipItem(UInventoryComponent* Inventory, int32 InventoryIndex);

	UFUNCTION(BlueprintCallable)
	bool Unequip(EEquipmentSlot Slot, UInventoryComponent* Inventory);

	UFUNCTION(BlueprintCallable)
	UItemDefinition* GetEquipment(EEquipmentSlot Slot) const;

	UFUNCTION(BlueprintCallable)
	bool IsSlotOccupied(EEquipmentSlot Slot) const;

private:

	FEquipmentSlotData* FindSlot(EEquipmentSlot Slot);
	const FEquipmentSlotData* FindSlot(EEquipmentSlot Slot) const;

	bool ApplyEquipEffect(UItemDefinition* Item, FEquipmentSlotData& SlotData);
	void RemoveEquipEffect(FEquipmentSlotData& SlotData);
};
