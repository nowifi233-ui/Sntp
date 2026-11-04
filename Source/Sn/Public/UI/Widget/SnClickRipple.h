// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnClickRipple.generated.h"

/**
 * Click ripple effect, separate from the software cursor: the engine clips
 * software cursors to their size, so a ripple expanding past the icon would
 * be cut off. This widget is a normal top-level widget positioned at the
 * cursor on click (no per-frame following, so no lag) and plays the ripple
 * animation in BP. Must be Hit Test Invisible in the designer.
 */
UCLASS()
class SN_API USnClickRipple : public USnUserWidget
{
	GENERATED_BODY()

public:
	/** Positions the ripple at a viewport position and plays the animation. */
	UFUNCTION(BlueprintCallable, Category = "Cursor")
	void PlayAt(const FVector2D& ViewportPosition);

protected:
	/** BP: play the ripple animation (scale up + fade out). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Cursor")
	void OnPlayRipple();
};
