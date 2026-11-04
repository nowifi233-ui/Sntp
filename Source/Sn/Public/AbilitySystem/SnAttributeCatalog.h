// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SnAttributeCatalog.generated.h"

/**
 * Metadata for one attribute: its gameplay tag identity, the GAS attribute
 * behind it, and how to display it.
 */
USTRUCT(BlueprintType)
struct FSnAttributeInfo
{
	GENERATED_BODY()

	/** Identity shared across systems (UI rows, effects, icons). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	FGameplayTag Tag;

	/** The GAS attribute this entry reads/writes. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	FGameplayAttribute Attribute;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * Single source of truth for attribute metadata. Referenced from Sn Settings;
 * UI rows resolve a tag against this catalog and auto-bind the attribute.
 */
UCLASS(BlueprintType)
class SN_API USnAttributeCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Finds the catalog entry for a tag, or nullptr. */
	const FSnAttributeInfo* FindInfoByTag(const FGameplayTag& Tag) const;

	/** Finds the catalog entry for a GAS attribute (reverse lookup, used when
	 *  reading equip-effect modifiers for the item detail panel), or nullptr. */
	const FSnAttributeInfo* FindInfoByAttribute(const FGameplayAttribute& Attribute) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes")
	TArray<FSnAttributeInfo> Attributes;
};
