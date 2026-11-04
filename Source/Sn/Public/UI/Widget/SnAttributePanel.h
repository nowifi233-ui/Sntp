// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnAttributePanel.generated.h"

class UWidget;

/**
 * Attribute panel base: draggable by grabbing the DragHandleArea (a widget
 * named "DragHandleArea" in the designer, e.g. a header bar Border).
 *
 * Dragging moves the panel via Render Transform translation instead of the
 * viewport slot position: SetPositionInViewport's DPI handling and the
 * absolute-vs-viewport coordinate mismatch are the classic causes of drag
 * jumps, while render transforms sidestep both. Movement is computed from
 * the delta between mouse events (same coordinate space), so no absolute
 * position lookups are involved.
 */
UCLASS()
class SN_API USnAttributePanel : public USnUserWidget
{
	GENERATED_BODY()

public:
	/** Area that starts a drag on press. Auto-bound to the widget named
	 *  "DragHandleArea" in the designer (BindWidget), e.g. a header bar Border. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (BindWidget), Category = "AttributePanel")
	TObjectPtr<UWidget> DragHandleArea;

protected:
	//~ Begin UUserWidget
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	//~ End UUserWidget

private:
	UPROPERTY()
	bool bDragging = false;

	/** Cursor position at grab time; drag movement is the delta from it (same space). */
	FVector2D StartCursorPos = FVector2D::ZeroVector;

	/** Render translation at grab time; the base the drag delta is added to. */
	FVector2D StartTranslation = FVector2D::ZeroVector;
};
