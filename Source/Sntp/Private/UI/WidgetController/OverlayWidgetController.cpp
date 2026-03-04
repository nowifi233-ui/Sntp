// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
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

	Cast<USntpAbilitySystemComponent>(AbilitySystemComponent)->OnEffectApplied.AddLambda(
		[](const FGameplayTagContainer& TagContainer)
		{
			for (const FGameplayTag& Tag : TagContainer)
			{
				// Broadcast the tag to widget controller
				const FString Msg = FString::Printf(TEXT("GE Tag: %s"), *Tag.ToString());
				GEngine->AddOnScreenDebugMessage(1, 5.f, FColor::Green, Msg);
			}
		});
}

void UOverlayWidgetController::OnHealthChangedFunc(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue);
}

void UOverlayWidgetController::OnMaxHealthChangedFunc(const FOnAttributeChangeData& Data) const
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
}


