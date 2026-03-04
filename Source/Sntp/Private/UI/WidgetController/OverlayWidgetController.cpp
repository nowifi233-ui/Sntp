// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/SntpAttributeSet.h"

void UOverlayWidgetController::BroadcastInitialValue()
{
	const USntpAttributeSet* SntpAttributeSet = CastChecked<USntpAttributeSet>(AttributeSet);
	OnHealthChanged.Broadcast(SntpAttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(SntpAttributeSet->GetMaxHealth());
	OnManaChanged.Broadcast(SntpAttributeSet->GetMana());
}

void UOverlayWidgetController::BindCallbackToDependencies()
{
	const USntpAttributeSet* SntpAttributeSet = CastChecked<USntpAttributeSet>(AttributeSet);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetHealthAttribute()).AddUObject(this, &UOverlayWidgetController::OnHealthChangedFunc);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetMaxHealthAttribute()).AddUObject(this, &UOverlayWidgetController::OnMaxHealthChangedFunc);
}

void UOverlayWidgetController::OnHealthChangedFunc(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnMaxHealthChangedFunc(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
}


