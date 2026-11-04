// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Window/SnActivatableWindow.h"
#include "InputCoreTypes.h"
#include "Player/SnPlayerController.h"
#include "UI/Widget/SnInventorySlotWidget.h"
#include "UI/WidgetController/SnWidgetController.h"

FReply USnActivatableWindow::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Click-to-raise: only unhandled empty-area clicks land here (clickable
	// children like buttons return Handled first). An empty-area click also
	// cancels any carried item (clicking empty space = cancel, not drop).
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetOwningPlayer()))
		{
			USnInventorySlotWidget::CancelCarry();
			SnPC->RaiseWindow(this);
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USnActivatableWindow::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Debounce: the OS repeats key-down while a key is held. Repeats are
	// swallowed here so a held B/C toggles its window exactly once - the
	// per-window BP OnKeyDown handlers (B/C toggles) would otherwise flip the
	// window open/closed continuously. Also covers a held Escape.
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

	// Escape closes the topmost window (the focused one - the manager keeps
	// focus on the topmost). Handled only when a window actually closed, so
	// the key can bubble to parents otherwise.
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetOwningPlayer()))
		{
			// Closing the window also ends any carried item (the carry session
			// lives on the slots; a closed grid has nowhere to drop).
			USnInventorySlotWidget::CancelCarry();
			if (SnPC->CloseWindow())
			{
				return FReply::Handled();
			}
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USnActivatableWindow::SetWidgetController(UObject* InController)
{
	if (WidgetController == InController)
	{
		// Same source re-pushed (resident instance re-opened): bindings are still
		// live from the last open - only refresh the initial values. Re-firing
		// WidgetControllerSet here would make BP bind its delegates twice.
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
