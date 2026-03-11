// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataAsset.h"
#include "CharacterClassInfo.generated.h"


class UGameplayEffect;
/**
 *
 */
UENUM(BlueprintType)
enum class ECharacterClass : uint8
{
	Elementalist,
	Warrior,
	Ranger
};

/*
 *
 */
USTRUCT(BlueprintType)
struct FCharacterClassDefaultInfo
{
	GENERATED_USTRUCT_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category="class default")
	TSubclassOf<UGameplayEffect> PrimaryAttribute;
};

/**
 * 
 */
UCLASS()
class SNTP_API UCharacterClassInfo : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, Category="class default")
	TMap<ECharacterClass, FCharacterClassDefaultInfo> CharacterClassInformation;
	
	UPROPERTY(EditDefaultsOnly, Category="comon class default")
	TSubclassOf<UGameplayEffect> VitalAttribute;
	
	UPROPERTY(EditDefaultsOnly, Category="comon class default")
	TSubclassOf<UGameplayEffect> SecondaryAttribute;
	
	UPROPERTY(EditDefaultsOnly, Category="comon class default")
	TArray<TSubclassOf<UGameplayAbility>> CommonAbilities;
	
	FCharacterClassDefaultInfo GetClassDefaultInfo(ECharacterClass CharacterClass);
};
