// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/SntpAttributeSet.h"

void UOverlayWidgetController::BroadcastInitialValue()
{
	const USntpAttributeSet* SntpAttributeSet = CastChecked<USntpAttributeSet>(AttributeSet);
	OnHealthChanged.Broadcast(SntpAttributeSet->GetHealth());
	OnManaChanged.Broadcast(SntpAttributeSet->GetMana());
}
