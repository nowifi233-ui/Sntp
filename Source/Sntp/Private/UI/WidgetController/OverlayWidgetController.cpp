// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/OverlayWidgetController.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "UI/SntpHUD.h"

class ASntpHUD;

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
	
	// Init 
	if (const APawn* Pawn = PlayerController->GetPawn())
	{
		InventoryComponent = Pawn->FindComponentByClass<UInventoryComponent>();
	}
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnHealthChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxHealthChanged.Broadcast(Data.NewValue);
		});
	
	BindAttributeChangedDelegate();
	
	// Bind AbilitySystemComponent Attribute Changed

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
	
	if (InventoryComponent)
	{
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &UOverlayWidgetController::OnInventoryChanged);
	}
}

void UOverlayWidgetController::RequestToggleHUD()
{
	PlayerController->GetHUD<ASntpHUD>()->ToggleBag(PlayerController);
}

void UOverlayWidgetController::OnInventoryChanged()
{
	OnInventoryUpdate.Broadcast();
}

void UOverlayWidgetController::OnInteractedOptionSelected(FInteractionOption Option)
{
	OnOptionSelected.Broadcast(Option);
}


void UOverlayWidgetController::BindAttributeChangedDelegate()
{
	const USntpAttributeSet* SntpAttributeSet = CastChecked<USntpAttributeSet>(AttributeSet);
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetAttackAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnAttackChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetDefenseAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnDefenseChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetElementalMasteryAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnElementalMasteryChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetDamageBonusAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnDamageBonusChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetCriticalChanceAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnCriticalChanceChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetCriticalDamageAttribute()).AddLambda(
	[this](const FOnAttributeChangeData& Data)
		{
			OnCriticalDamageChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetMaxResilienceAttribute()).AddLambda(
	[this](const FOnAttributeChangeData& Data)
		{
			OnMaxResilienceChanged.Broadcast(Data.NewValue);
		});
	
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetResilienceAttribute()).AddLambda(
	[this](const FOnAttributeChangeData& Data)
		{
			OnResilienceChanged.Broadcast(Data.NewValue);
		});
	
}

