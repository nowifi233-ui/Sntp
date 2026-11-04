// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnCursorWidget.generated.h"

/**
 * Software cursor. The engine follows the mouse automatically once the HUD
 * registers it via SetSoftwareCursorWidget - no Tick positioning, no lag.
 * The click effect is driven from input (player controller -> HUD ->
 * PlayClickEffect), which plays the ripple animation in BP.
 */
UCLASS()
class SN_API USnCursorWidget : public USnUserWidget
{
	GENERATED_BODY()

public:
	/** Plays the click effect (ripple in BP). Called on left-click. */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void PlayClickEffect();

	/** Shows the carried item's icon on the cursor (click-pickup; the engine
	 *  follows the mouse with the software cursor, so the icon tracks for
	 *  free - no Tick positioning). */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void SetCarriedItem(FName ItemRowName);

	/** Hides the carried item icon. */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void ClearCarriedItem();

protected:
	/** BP: play the ripple / particle animation here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cursor")
	void OnClickEffect();

	/** BP: the carried item icon changed - show the item icon (look it up via
	 *  the ItemLibrary) or hide it on NAME_None. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cursor")
	void OnCarriedItemChanged(FName ItemRowName);
};
