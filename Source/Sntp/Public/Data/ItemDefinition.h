// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "Engine/DataAsset.h"
#include "ItemDefinition.generated.h"

UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
	None,
	Weapon,
	Head,
	Chest,
	Legs,
	Feet,
	Ring1,
	Ring2,
	Necklace,
};
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SNTP_API UItemDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FName ItemID;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText Name;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FText Description;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag ItemType;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag Rarity;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FGameplayTagContainer ItemTags;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bStackable = true;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stack", meta=(ClampMin=1))
	int32 MaxStack = 99;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> UseEffect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TSubclassOf<UGameplayAbility> UseAbility;
	
	/* Equipment component */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	EEquipmentSlot EquipSlot;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bEquipable = false;
};
