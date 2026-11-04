// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnInventorySlotWidget.generated.h"

class UDragDropOperation;

/**
 * Base class for inventory item slots (bag slots + equipment grid cells).
 *
 * Click-pickup reuses the classic drag-drop machinery: a press on this slot
 * IMMEDIATELY begins a real drag session (no movement threshold - the C++
 * NativeOnMouseButtonDown calls BeginDragDrop, the same path the engine takes
 * after DragDetected). The slot BP supplies the operation via GetDragOperation,
 * so the payload/class is whatever the existing drag already uses:
 *
 *   - press an item slot      -> "pickup" op  -> release on the source slot
 *     itself (self-drop) resolves as PICK UP, not a move
 *   - press while carrying    -> "carried" op -> self-drop resolves as DROP
 *     the carried item (its source data is the static carry state)
 *   - press and drag elsewhere-> the exact classic drag move, resolved by the
 *     UNCHANGED existing OnDrop code (no function extraction needed - the
 *     click paths are just extra op types in the same OnDrop)
 *
 * The carry state between the two clicks is static slot-level data (one
 * session per game, like Slate's global drag session): the item follows the
 * cursor via the HUD cursor icon, the source slot dims itself (BP) and
 * restores on OnCarryEnded. Empty-area clicks and Esc cancel (see
 * SnActivatableWindow). No controller involvement - pure UI-layer code.
 *
 * NOTE: the session is static (single-player UI). Two local players in one
 * PIE process would share it - key it per viewport if that ever happens.
 */
UCLASS()
class SN_API USnInventorySlotWidget : public USnUserWidget
{
	GENERATED_BODY()

public:
	/** Left-press = immediately begin a drag session (no threshold) when the
	 *  slot BP's GetDragOperation provides an operation. Unhandled otherwise
	 *  (empty idle slot), so the slot BP's own mouse-down handling still runs.
	 *  The slot BP's OnMouseButtonDown/OnDragDetected are bypassed while a
	 *  session starts here (the reply is Handled). */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Starts carrying the item - call from the slot BP's OnDrop when it sees
	 *  a pickup-marked self-drop (this slot is the source). Records the static
	 *  carry state and shows the item on the HUD cursor icon. No-op while a
	 *  session is already active. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void PickUpItem(FName ItemRowName, int32 SourceSlot, int32 SourceGrid);

	/** Ends the session without dropping (empty-area click, Esc, window
	 *  close) - or call from the drop target's OnDrop after a successful
	 *  carried-item drop. Restores the source slot and hides the cursor icon. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static void CancelCarry();

	/** Whether a carry session is active. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool IsCarrying();

	/** Carried item row name (invalid while not carrying). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static FName GetCarriedRowName();

	/** Source slot of the carried item (bag index or grid cell number). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static int32 GetCarriedSourceSlot();

	/** Source grid of the carried item (0 = bag, 1 = equipment grid). */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static int32 GetCarriedSourceGrid();

protected:
	/** BP: the operation for a press on this slot, or nullptr for no session
	 *  (empty slot while not carrying). The op's class/payload marks its
	 *  intent - the slot's OnDrop branches on it:
	 *   - this slot holds an item: pickup op (self-drop = pick up; source
	 *     data in the payload)
	 *   - a carry session is active: carried op (self-drop = drop it here;
	 *     source data from the GetCarried* statics)
	 *  Use the SAME op class/payload the classic drag used, so the existing
	 *  OnDrop move resolution stays untouched. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	UDragDropOperation* GetDragOperation();

	/** BP: the carry session ended (dropped or cancelled) - restore this (the
	 *  SOURCE) slot's visual. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory")
	void OnCarryEnded();

private:
	/** The source slot of the active session (invalid while none - also
	 *  self-heals when the slot dies, e.g. level travel). */
	static TWeakObjectPtr<USnInventorySlotWidget> CarriedFromSlot;

	static FName CarriedRowName;
	static int32 CarriedSourceSlot;
	static int32 CarriedSourceGrid;
};
