// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "SnWidgetController.generated.h"

class APlayerController;
class APlayerState;
class UAbilitySystemComponent;
class USnAttributeSet;

/**
 * Base class for UI controllers. Owned by the player controller (created
 * lazily on first access), handed the GAS data sources via Initialize.
 * Widgets bind to this controller's delegates and never touch GAS directly.
 */
UCLASS(BlueprintType)
class SN_API USnWidgetController : public UObject
{
	GENERATED_BODY()

public:
	/** Hands over the data sources, then binds callbacks. */
	void Initialize(APlayerController* InPC);

	/** Push current attribute values. Called by the widget AFTER it has bound the delegates. */
	virtual void BroadcastInitialValues() {}

	/** Read-only access to the ASC, for widgets that self-bind (e.g. attribute rows). */
	UFUNCTION(BlueprintPure, Category = "WidgetController")
	UAbilitySystemComponent* GetAbilitySystemComponent() const { return AbilitySystemComponent; }

protected:
	/** Override: bind to ASC / attribute delegates. Called from Initialize. */
	virtual void BindCallbacks() {}

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerController> PlayerController;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<APlayerState> PlayerState;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<USnAttributeSet> AttributeSet;
};
