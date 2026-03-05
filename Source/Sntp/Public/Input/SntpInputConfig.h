// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "InputTriggers.h"
#include "Engine/DataAsset.h"
#include "SntpInputConfig.generated.h"

class UInputAction;

USTRUCT(BlueprintType)
struct FSntpInputAction
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	const UInputAction* InputAction = nullptr;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag InputTag = FGameplayTag();
	
	UPROPERTY(EditDefaultsOnly)
	ETriggerEvent TriggerEvent = ETriggerEvent::None;
};

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SNTP_API USntpInputConfig : public UDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FSntpInputAction> SntpInputActions;
	
	const UInputAction* FindInputActionByTag(const FGameplayTag& InputTag, bool bLogNotFound = false) const;
};
