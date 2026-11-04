// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "UI/Widget/SnUserWidget.h"
#include "GameplayTagContainer.h"
#include "SnAttributeRow.generated.h"

class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * Designer-placed row for the attribute panel. Give it a tag (AttributeTag);
 * when the panel hands it the widget controller, it resolves the catalog
 * entry from Sn Settings and auto-binds the attribute's value changes.
 * Display name and value are pushed to BP events for layout.
 */
UCLASS(BlueprintType)
class SN_API USnAttributeRow : public USnUserWidget
{
	GENERATED_BODY()

public:
	/** Tag this row displays, e.g. Attribute.Health. Resolved against the catalog on init. */
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Attribute")
	FGameplayTag AttributeTag;

protected:
	//~ Begin USnUserWidget
	virtual void OnWidgetControllerSet() override;
	virtual void NativeDestruct() override;
	//~ End USnUserWidget

	/** BP: display name resolved from the catalog. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attribute")
	void OnAttributeInfoResolved(const FText& DisplayName);

	/** BP: update the displayed value. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Attribute")
	void OnAttributeValueChanged(float NewValue);

private:
	void HandleAttributeChanged(const FOnAttributeChangeData& Data);

	/** ASC we subscribed to; needed to unbind in NativeDestruct. */
	UPROPERTY()
	TObjectPtr<UAbilitySystemComponent> BoundASC;

	FGameplayAttribute BoundAttribute;
	FDelegateHandle ValueChangedHandle;
};
