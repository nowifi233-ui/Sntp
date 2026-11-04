// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SnWindowManager.generated.h"

class ASnPlayerController;
class USnActivatableWindow;

/**
 * Owns every game window: their resident instances, Z order, input mode and
 * keyboard focus. Community-style widget system - Lyra's UI-manager
 * responsibilities, minus CommonUI:
 *
 * - Resident instances: created on first open, Collapsed when closed, never
 *   destroyed. Re-opening re-injects the data source (idempotent
 *   SetWidgetController), so windows always show fresh data.
 * - Several windows visible at once (inventory + attribute panel + ...), each
 *   anchored to its own screen region; clicking a window raises it (top of the
 *   Z order) and refocuses it. Escape closes the topmost window.
 * - Input mode follows the open count: first window open -> FInputModeUIOnly +
 *   cursor; last window closed -> FInputModeGameOnly. The chat input line (not
 *   a window - see USnChatInputWidget) is the third state: UI-only focused on
 *   its text box, with the cursor still HIDDEN. All local client state -
 *   windows never replicate; every client owns its own manager.
 *
 * Lazy-created by the player controller (same pattern as the combat widget
 * controller); the PC's public window API forwards here, so blueprint call
 * sites stay untouched.
 */
UCLASS()
class SN_API USnWindowManager : public UObject
{
	GENERATED_BODY()

public:
	/** Opens the window of the given class and injects the data source. First
	 *  open creates the resident instance, later opens reuse it (Collapsed ->
	 *  Visible); opening an already-visible window just raises and refreshes it. */
	void OpenWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource);

	/** Closes the topmost visible window. Returns false when none was open
	 *  (callers should not consume the key/event). */
	bool CloseWindow();

	/** Opens the window if closed, closes it if open (DataSource unused on the
	 *  close path). */
	void ToggleWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource);

	/** Whether an instance of the class is currently visible. */
	bool IsWindowOpen(TSubclassOf<USnActivatableWindow> WidgetClass) const;

	/** Whether any window is currently visible. */
	bool IsAnyWindowOpen() const;

	/** Brings the window to the top of the Z order and refocuses it (called by
	 *  the window base on unhandled mouse-downs - empty areas; buttons handle
	 *  their own clicks and do not raise). */
	void RaiseWindow(USnActivatableWindow* Window);

private:
	friend class ASnPlayerController;

	/** Caches the owning controller (SetInputMode / cursor live on it). */
	void Initialize(ASnPlayerController* InPC);

	/** Shared close path for every caller: hide, notify, refocus, input mode. */
	void CloseWindowInstance(USnActivatableWindow* Window);

	/** Finds the resident instance of the class, or null. */
	USnActivatableWindow* FindWindow(TSubclassOf<USnActivatableWindow> WidgetClass) const;

	/** Schedules the input-mode switch for next frame (see ApplyInputModeNow). */
	void UpdateInputMode();

	/** Applies the input mode from the open-window count (and the chat line,
	 *  which outranks it - see the .cpp). Runs one frame after UpdateInputMode
	 *  so the key press that closed the last window - still bubbling through
	 *  Slate - never reaches the game's input stack, which would re-toggle the
	 *  window ("press B twice to close"). The chat line uses the same deferral
	 *  on close, for the same reason: the committing Enter is still on its way
	 *  up and would re-open the line. */
	void ApplyInputModeNow();

	/** Base Z order for windows (above the HUD at 0, below the click ripple at
	 *  100); each window sits at Base + its index, so the tail is always on top. */
	static const int32 WindowBaseZ = 20;

	/** The owning player controller. */
	UPROPERTY()
	TObjectPtr<ASnPlayerController> PC;

	/** Every window instance ever created, Z order ascending (tail = topmost). */
	UPROPERTY()
	TArray<TObjectPtr<USnActivatableWindow>> Windows;
};
