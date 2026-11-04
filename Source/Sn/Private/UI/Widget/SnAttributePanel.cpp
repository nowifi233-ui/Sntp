// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnAttributePanel.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/CanvasPanelSlot.h"
#include "InputCoreTypes.h"

FReply USnAttributePanel::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Preview fires along the ancestor chain before the child gets the click,
	// so the panel can intercept a press on its header bar.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& DragHandleArea
		&& DragHandleArea->GetCachedGeometry().IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
	{
		bDragging = true;
		// Base for the drag: cursor at grab time (same space as the deltas
		// that follow) and the panel's current render translation.
		StartCursorPos = InMouseEvent.GetScreenSpacePosition();
		StartTranslation = GetRenderTransform().Translation;
		// Capture: mouse events keep coming to this widget even when the
		// cursor leaves the panel, so the drag survives outside the bounds.
		return FReply::Handled().CaptureMouse(TakeWidget());
	}

	return FReply::Unhandled();
}

FReply USnAttributePanel::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		// Movement = cursor delta since grab. Both positions come from the
		// same event space, so no absolute/DPI conversion can skew them; only
		// the final delta is divided by the viewport scale once, since render
		// transform translations are in viewport units.
		const float ViewportScale = UWidgetLayoutLibrary::GetViewportScale(this);
		const FVector2D CursorDelta = InMouseEvent.GetScreenSpacePosition() - StartCursorPos;
		const FVector2D Delta = ViewportScale > 0.f ? CursorDelta / ViewportScale : CursorDelta;

		SetRenderTranslation(StartTranslation + Delta);
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply USnAttributePanel::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}

	return FReply::Unhandled();
}
