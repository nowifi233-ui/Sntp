// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InventoryComponent.h"
#include "Interaction/Interactable.h"
#include "UI/SntpHUD.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "OverlayWidgetController.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAttributeSignature, float, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FScrollDelegate, int32, NewValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOptionSelected, FInteractionOption, NewValue);
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

	//~ Delegates
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnMaxHealthChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnManaChanged;
	
	// Attributes
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnAttackChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnDefenseChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnElementalMasteryChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnCriticalChanceChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnCriticalDamageChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnDamageBonusChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnMaxResilienceChanged;
	
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FOnAttributeSignature OnResilienceChanged;
	
	// Other Delegates
	UPROPERTY(BlueprintAssignable, Category="GAS|Attributes")
	FScrollDelegate OnScrollDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FOnInventoryUpdate OnInventoryUpdate;
	
	UPROPERTY(BlueprintAssignable)
	FOnOptionSelected OnOptionSelected;
	
	UPROPERTY(BlueprintReadWrite)
	int32 CurrentOptionIndex;
	//~ Delegates End
	
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
	/**
	 * Interaction
	 */
	
	UPROPERTY(BlueprintReadWrite)
	FInteractionOption CurrentOption;
	
	/**
	 * Bag
	 */
	UFUNCTION(BlueprintCallable)
	void RequestToggleHUD();
	
	UPROPERTY(BlueprintReadWrite)
	ASntpHUD* SntpHUD;
	
private:
	UFUNCTION()
	void OnInventoryChanged();
	
	UFUNCTION()
	void OnInteractedOptionSelected(FInteractionOption Option);


private:
	void BindAttributeChangedDelegate();
};
