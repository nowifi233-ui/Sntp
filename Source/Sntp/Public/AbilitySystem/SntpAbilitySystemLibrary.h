// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SntpAbilitySystemLibrary.generated.h"

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
};
