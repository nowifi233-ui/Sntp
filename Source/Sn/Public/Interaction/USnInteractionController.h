// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/SnWidgetController.h"
#include "USnInteractionController.generated.h"

/** One row of the interaction UI: the target and its resolved display name. */
USTRUCT(BlueprintType)
struct FSnInteractionEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<AActor> Target;

	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	FText DisplayName;
};

/**
 * Transient controller for the interaction UI. Created per update by the
 * player controller, handed to the widget via SetWidgetController, destroyed
 * when the UI closes. Carries only display data - all logic runs in the
 * player controller and the action objects.
 */
UCLASS(BlueprintType)
class SN_API USnInteractionController : public USnWidgetController
{
	GENERATED_BODY()

public:
	/** Visible targets, in the perception layer's stable order. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TArray<FSnInteractionEntry> Entries;

	/** Index of the selected entry (mouse wheel cycles it). */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	int32 SelectedIndex = 0;
};
