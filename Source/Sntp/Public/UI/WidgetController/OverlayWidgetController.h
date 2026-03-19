// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InventoryComponent.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "OverlayWidgetController.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScrollDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOptionSelected, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdate);
/**
 * 
 */

UCLASS(Blueprintable, BlueprintType)
class SNTP_API UOverlayWidgetController : public USntpWidgetController
{
	GENERATED_BODY()
	
public:
	virtual void BroadcastInitialValue() override;
	virtual void BindCallbackToDependencies() override;

	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnManaChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FScrollDelegate OnScrollDelegate;
	
	UPROPERTY(BlueprintReadWrite)
	int32 CurrentOptionIndex;
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryUpdate OnInventoryUpdate;
	
	UPROPERTY(BlueprintAssignable)
	FOnOptionSelected OnOptionSelected;
	
private:
	UFUNCTION()
	void OnInventoryChanged();
	
	UFUNCTION()
	void OnInteractedOptionSelected(int32 Index);
};
