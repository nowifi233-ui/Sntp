// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/SntpWidgetController.h"
#include "OverlayWidgetController.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeSignature, float, NewValue);

/**
 * 
 */

UCLASS(Blueprintable, BlueprintType)
class SNTP_API UOverlayWidgetController : public USntpWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValue() override;

	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnManaChanged;
};
