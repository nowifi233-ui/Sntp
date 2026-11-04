// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/SnUserWidget.h"
#include "InputCoreTypes.h"
#include "Player/SnPlayerController.h"
#include "UI/WidgetController/SnWidgetController.h"

FReply USnUserWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Debounce: the OS repeats key-down while a key is held - swallow the
	// repeats so a held Escape closes at most one window (matching
	// USnActivatableWindow, which also feeds BP OnKeyDown toggles).
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

	// Window widgets (inventory, config menus) close on Escape. UIOnly input
	// mode keeps keyboard events inside UMG - Enhanced Input never sees them -
	// so closing happens here, once per base class, not per-widget BP overrides.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (ASnPlayerController* PC = Cast<ASnPlayerController>(GetOwningPlayer()))
		{
			// Only consume the key when a window was actually closed. Without a
			// window the event falls through (Super) so other UI - e.g. the
			// attribute panel's GameAndUI focus - or the game's own Escape
			// binding can still react to it.
			if (PC->CloseConfigWindow())
			{
				return FReply::Handled();
			}
		}
	}

	// Non-Escape keys fall through so BP OnKeyDown overrides (e.g. B-key
	// toggle in the inventory) still run.
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USnUserWidget::SetWidgetController(UObject* InController)
{
	if (WidgetController == InController)
	{
		// Same controller re-injected (a resident window re-opened with its
		// controller, e.g. the attribute panel): bindings are still live -
		// refresh initial values only, never re-fire the event (BP would
		// AddDynamic a second time and double-bind).
		if (USnWidgetController* Controller = Cast<USnWidgetController>(WidgetController))
		{
			Controller->BroadcastInitialValues();
		}
		return;
	}

	WidgetController = InController;
	OnWidgetControllerSet();
	WidgetControllerSet();

	// Order matters: broadcast only after the widget has bound its delegates,
	// otherwise the initial values would be lost. Generic data sources (e.g. a
	// spawner actor) have no initial-value push - they refresh themselves.
	if (USnWidgetController* Controller = Cast<USnWidgetController>(WidgetController))
	{
		Controller->BroadcastInitialValues();
	}
}
