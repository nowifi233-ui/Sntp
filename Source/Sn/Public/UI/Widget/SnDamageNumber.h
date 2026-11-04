// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "Components/RichTextBlock.h"
#include "CoreMinimal.h"
#include "Engine/TimerHandle.h"
#include "SnDamageNumber.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSnDamageNumberShownDelegate);

/**
 * Screen-space floating damage number. Positioned at a world location on
 * creation, then handed to the blueprint: ShowAt fires OnShown so the WBP can
 * play its float animation - the motion and the fade live in that animation,
 * the C++ side never writes position or opacity after the initial placement,
 * so the animation is never overridden. The animation ends by calling
 * FinishNumber, which collapses the number and returns it to the pool.
 * Deliberately independent of the damaged actor: numbers keep floating when
 * the enemy dies, and never jitter with its movement.
 *
 * The text is a RichTextBlock driven by a style tag ("Damage.Physical",
 * "Damage.Fire", "Damage.Ice", "Damage.Lightning", "Damage.Miss", "XP" - rows
 * in the widget's TextStyleSet data table), so element colors live in the
 * style table, not in code. Consecutive numbers (e.g. the physical + enchant
 * hits of one attack) are scattered deterministically on a golden-angle ring,
 * so they never stack on top of each other.
 */
UCLASS(Abstract, Blueprintable)
class SN_API USnDamageNumber : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Projects the world location to screen, resets the pooled leftovers
	 *  (transform, opacity), sets the styled number, then fires OnShown for the
	 *  blueprint to play its float animation. InStyleTag names the rich-text
	 *  style row ("Damage.Physical" etc.); bPrefixPlus renders "+N" (XP),
	 *  bAppendBang appends "!" (crits), InTextOverride replaces the numeric
	 *  text ("MISS" for dodges). ScatterIndex drives the deterministic ring
	 *  offset so nearby numbers (one attack's physical + enchant hits) separate
	 *  instead of stacking. */
	void ShowAt(const FVector& WorldLocation, float Amount, const FName InStyleTag, bool bPrefixPlus = false, bool bAppendBang = false, const FString& InTextOverride = TEXT(""), int32 ScatterIndex = 0);

	/** Ends the number: collapses it and returns it to the pool. The only exit
	 *  path - the float animation calls this when it finishes. */
	UFUNCTION(BlueprintCallable, Category = "DamageNumber")
	void FinishNumber();

	/** Fired right after ShowAt displayed the number (pooled reuse included).
	 *  Blueprint: bind the float animation here (play on the resident widget
	 *  instance - the pool gives the same animation every show). Playback
	 *  resets on each broadcast. */
	UPROPERTY(BlueprintAssignable, Category = "DamageNumber")
	FSnDamageNumberShownDelegate OnShown;

	/** Called when the number finishes and should return to its pool. */
	FSimpleDelegate OnFinished;

protected:
	/** Empty by design - keeps the widget ticking (see the cpp: the 5.6+ UMG
	 *  animation tick manager disables animations on widgets that do not tick).
	 *  Motion and fade live in the blueprint animation. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Ring radius for ScatterIndex 0 (px); every 4th number steps out. */
	/** Ring radius for ScatterIndex 0 (px); every 4th number steps out. */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float ScatterBaseRadius = 26.f;

	/** Extra ring radius per scatter step (px). */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float ScatterRadiusStep = 16.f;

	/** Fallback: force-finish after this long if the blueprint animation never
	 *  calls FinishNumber (an unconfigured WBP would otherwise pin a number on
	 *  screen forever, exhausting the pool). */
	UPROPERTY(EditDefaultsOnly, Category = "DamageNumber")
	float MaxLifetime = 3.f;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<URichTextBlock> NumberText;

private:
	/** Timeout: the float animation never finished - force the pool return. */
	void HandleTimeout();

	FTimerHandle FinishTimerHandle;
};
