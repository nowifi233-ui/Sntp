// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SnActivatableWindow.generated.h"

class USnWidgetController;

/**
 * Base class for every game window (inventory, storage, crafting, spawner menu,
 * attribute panel). Windows are managed by the PC's USnWindowManager: resident
 * instances (Collapsed when closed), several visible at once, click-to-raise,
 * Escape closes the topmost window. Every window is a plain UMG widget - no
 * CommonUI.
 *
 * Data injection (SetWidgetController) must be idempotent: a re-opened window
 * refreshes initial values without re-firing WidgetControllerSet (bindings
 * would double up).
 */
UCLASS()
class SN_API USnActivatableWindow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Injects the data source (same contract as USnUserWidget). Idempotent:
	 *  re-injecting the SAME controller only refreshes initial values - resident
	 *  instances are re-opened with the same source and must not double-bind. */
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	void SetWidgetController(UObject* InController);

	UFUNCTION(BlueprintPure, Category = "WidgetController")
	UObject* GetWidgetController() const { return WidgetController; }

	/** Called by the window manager after the window has been closed (hidden).
	 *  BP hook: unpause the spawner menu, reset per-open state, ... */
	UFUNCTION(BlueprintImplementableEvent, Category = "Window")
	void OnWindowClosed();

	/** Called by the window manager every time the window is opened (visible).
	 *  BP hook: PlayAnimation(open animation), an open sound cue, ...
	 *  Symmetric to OnWindowClosed - and per OPEN, not per instance: windows are
	 *  resident (collapsed when closed, never destroyed), so Construct /
	 *  AddedToViewport fire once for the lifetime of the process and cannot be
	 *  used for this. The widget is already visible, raised and populated when
	 *  this fires - UMG animations only advance while a widget is visible
	 *  (UMGSequenceTickManager gates on IsVisible()), so playing from here is
	 *  the only correct moment; playing before the window is shown would leave
	 *  the animation frozen and then drop it. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Window")
	void OnWindowOpened();

protected:
	/** Click-to-raise: an unhandled mouse-down (empty areas - buttons handle
	 *  their own clicks) brings this window to the top and refocuses it. */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Escape closes the topmost window (the focused one - focus follows the
	 *  topmost via the manager). Returns Handled only when a window closed. */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** C++ hook fired before the BP WidgetControllerSet event. Native subclasses
	 *  resolve data and bind here. */
	virtual void OnWidgetControllerSet() {}

	/** BP override: bind to the data source's delegates here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "WidgetController")
	void WidgetControllerSet();

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<UObject> WidgetController;
};
