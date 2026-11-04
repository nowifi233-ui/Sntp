// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnInventorySlotWidget.h"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Game/SnHUD.h"
#include "InputCoreTypes.h"
#include "Player/SnPlayerController.h"
#include "Slate/SObjectWidget.h"
#include "Slate/UMGDragDropOp.h"

TWeakObjectPtr<USnInventorySlotWidget> USnInventorySlotWidget::CarriedFromSlot;
FName USnInventorySlotWidget::CarriedRowName;
int32 USnInventorySlotWidget::CarriedSourceSlot = INDEX_NONE;
int32 USnInventorySlotWidget::CarriedSourceGrid = INDEX_NONE;

FReply USnInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// The slot BP decides whether this press starts a session and what it
		// carries: an item press = pickup op, an active carry = carried op, an
		// empty idle slot = nullptr (no session - fall through to the BP's own
		// mouse-down handling).
		UDragDropOperation* Operation = GetDragOperation();
		if (Operation)
		{
			// Immediate, threshold-less session - the press IS a drag. Same
			// construction the engine uses after DragDetected
			// (SObjectWidget::OnDragDetected): release on this slot = self-drop
			// = pick up / drop (the slot BP's OnDrop resolves it by op type);
			// release elsewhere = the classic drag move.
			TSharedPtr<SObjectWidget> SourceWidget = StaticCastSharedPtr<SObjectWidget>(GetCachedWidget());
			if (SourceWidget)
			{
				FVector2D ScreenCursorPos = InMouseEvent.GetScreenSpacePosition();
				FVector2D ScreenDrageePosition = InGeometry.GetAbsolutePosition();
				const float DPIScale = UWidgetLayoutLibrary::GetViewportScale(this);

				TSharedRef<FUMGDragDropOp> DragDropOp = FUMGDragDropOp::New(
					Operation,
					InMouseEvent.GetPointerIndex(),
					ScreenCursorPos,
					ScreenDrageePosition,
					DPIScale,
					SourceWidget);

				return FReply::Handled().BeginDragDrop(DragDropOp);
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USnInventorySlotWidget::PickUpItem(FName ItemRowName, int32 SourceSlot, int32 SourceGrid)
{
	// A session is already active: this self-drop is a drop target, not a
	// pickup (the BP only takes this branch when idle, kept as a guard).
	if (IsCarrying())
	{
		return;
	}

	CarriedFromSlot = this;
	CarriedRowName = ItemRowName;
	CarriedSourceSlot = SourceSlot;
	CarriedSourceGrid = SourceGrid;

	// The item follows the cursor through the HUD cursor icon; the calling BP
	// dims this slot's own visual.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ASnHUD* SnHUD = Cast<ASnHUD>(PC->GetHUD()))
		{
			SnHUD->ShowCarriedItem(ItemRowName);
		}
	}
}

void USnInventorySlotWidget::CancelCarry()
{
	// Self-healing: the source slot may already be gone (level travel) - the
	// weak ref simply invalidates and IsCarrying turns false.
	if (USnInventorySlotWidget* SourceSlot = CarriedFromSlot.Get())
	{
		SourceSlot->OnCarryEnded();

		if (APlayerController* PC = SourceSlot->GetOwningPlayer())
		{
			if (ASnHUD* SnHUD = Cast<ASnHUD>(PC->GetHUD()))
			{
				SnHUD->HideCarriedItem();
			}
		}
	}

	CarriedFromSlot = nullptr;
	CarriedRowName = NAME_None;
	CarriedSourceSlot = INDEX_NONE;
	CarriedSourceGrid = INDEX_NONE;
}

bool USnInventorySlotWidget::IsCarrying()
{
	return CarriedFromSlot.IsValid();
}

FName USnInventorySlotWidget::GetCarriedRowName()
{
	return CarriedRowName;
}

int32 USnInventorySlotWidget::GetCarriedSourceSlot()
{
	return CarriedSourceSlot;
}

int32 USnInventorySlotWidget::GetCarriedSourceGrid()
{
	return CarriedSourceGrid;
}
