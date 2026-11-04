// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnAttributeRow.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeCatalog.h"
#include "Game/SnSettings.h"
#include "UI/WidgetController/SnWidgetController.h"

void USnAttributeRow::OnWidgetControllerSet()
{
	Super::OnWidgetControllerSet();

	// SetWidgetController may run more than once (self-injection here, then a
	// panel forwarding its own controller) - re-init cleanly, no double binds.
	if (BoundASC && ValueChangedHandle.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttribute).Remove(ValueChangedHandle);
		ValueChangedHandle.Reset();
		BoundASC = nullptr;
	}

	if (!AttributeTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("USnAttributeRow: no AttributeTag set on %s"), *GetName());
		return;
	}

	const USnAttributeCatalog* Catalog = nullptr;
	if (const USnSettings* Settings = USnSettings::Get())
	{
		Catalog = Settings->GetAttributeCatalog();
	}

	const FSnAttributeInfo* Info = Catalog ? Catalog->FindInfoByTag(AttributeTag) : nullptr;
	if (!Info)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnAttributeRow: no catalog entry for tag %s (check Sn Settings)"), *AttributeTag.ToString());
		return;
	}

	OnAttributeInfoResolved(Info->DisplayName);

	// The injected source is a USnWidgetController for GAS-driven rows.
	UAbilitySystemComponent* ASC = WidgetController ? Cast<USnWidgetController>(WidgetController)->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return;
	}

	BoundASC = ASC;
	BoundAttribute = Info->Attribute;

	// Pull the current value, then subscribe to future changes.
	OnAttributeValueChanged(BoundASC->GetNumericAttribute(BoundAttribute));
	ValueChangedHandle = BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttribute)
		.AddUObject(this, &USnAttributeRow::HandleAttributeChanged);
}

void USnAttributeRow::HandleAttributeChanged(const FOnAttributeChangeData& Data)
{
	OnAttributeValueChanged(Data.NewValue);
}

void USnAttributeRow::NativeDestruct()
{
	if (BoundASC && ValueChangedHandle.IsValid())
	{
		BoundASC->GetGameplayAttributeValueChangeDelegate(BoundAttribute).Remove(ValueChangedHandle);
		ValueChangedHandle.Reset();
		BoundASC = nullptr;
	}

	Super::NativeDestruct();
}
