// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGameplayAbility.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "SnAbilityCatalog.generated.h"

class UTexture2D;

/**
 * Metadata for one ability: its identity tag, the ability class behind it,
 * and how to display it (icon + name). The description TEXT is deliberately
 * not stored here - the ability itself generates it via
 * USnGameplayAbility::GetDescription, because the ability owns the numbers
 * (damage, range, cooldown) and knows how they scale with level. This asset
 * owns the UI-facing static bits (identity, name, icon) - the same split as
 * the attribute catalog.
 */
USTRUCT(BlueprintType)
struct FSnAbilityInfo
{
	GENERATED_BODY()

	/** Identity shared across systems (UI rows, tooltips, effects). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FGameplayTag Tag;

	/** The ability class this entry describes (CDO data + GetDescription). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSubclassOf<USnGameplayAbility> AbilityClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Ability")
	TSoftObjectPtr<UTexture2D> Icon;
};

/**
 * Single source of truth for ability metadata. Referenced from Sn Settings;
 * the ability widget controller resolves granted abilities against this
 * catalog before assembling UI data.
 */
UCLASS(BlueprintType)
class SN_API USnAbilityCatalog : public UDataAsset
{
	GENERATED_BODY()

public:
	/** Finds the catalog entry for a tag, or nullptr. */
	const FSnAbilityInfo* FindInfoByTag(const FGameplayTag& Tag) const;

	/** Finds the catalog entry for an ability class, or nullptr. */
	const FSnAbilityInfo* FindInfoByClass(const TSubclassOf<USnGameplayAbility>& InAbilityClass) const;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Abilities")
	TArray<FSnAbilityInfo> Abilities;
};
