// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SntpAbilitySystemLibrary.generated.h"

struct FInteractionOption;
class UAbilitySystemComponent;
enum class ECharacterClass : uint8;
/**
 * 
 */
UCLASS()
class SNTP_API USntpAbilitySystemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable, Category="SntpAttributeSystemLibrary")
	static void InitializeDefaultAttributes(const UObject* WorldContextObject, ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC);

	UFUNCTION(BlueprintCallable, Category="SntpAttributeSystemLibrary")
	static void GiveStartupAbilites(const UObject* WorldContextObject, UAbilitySystemComponent* ASC);
	
	UFUNCTION(BlueprintPure, Category="SntpAttributeSystemLibrary")
	static bool IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle);
	
	UFUNCTION(BlueprintPure, Category="SntpAttributeSystemLibrary")
	static FText FormatTime(int32 TotalSeconds);
	
	
	UFUNCTION(BlueprintPure, Category="SntpAttributeSystemLibrary")
	static AActor* GetNearestEnemy(AActor* Source, float Radius);
	
	UFUNCTION(BlueprintPure, Category="SntpAttributeSystemLibrary")
	static bool IsEnemy(AActor* Source, AActor* Target);
	
	UFUNCTION(BlueprintPure, Category="SntpAttributeSystemLibrary")
	static bool IsSameInteractionOption(const FInteractionOption& A, const FInteractionOption& B);
};
