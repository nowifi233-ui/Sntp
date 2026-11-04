// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/WidgetController/SnWidgetController.h"
#include "USnDialogueController.generated.h"

class UTexture2D;
struct FSnDialogueRow;

/**
 * Data-only transient controller for one dialogue session. The PC creates one
 * per PlayDialogue and Start()s it on the first row; the dialogue window
 * pulls the current row through the getters and drives Advance() per click -
 * the window never touches the data table itself (same pull pattern as the
 * other widget controllers, minus the delegate plumbing: the line only moves
 * on an explicit Advance, so there is nothing to subscribe to).
 */
UCLASS(BlueprintType)
class SN_API USnDialogueController : public USnWidgetController
{
	GENERATED_BODY()

public:
	/** Opens the dialogue at FirstRow. A missing row / unset table logs a
	 *  warning and leaves the dialogue empty (GetCurrentRow null, Advance
	 *  returns false). */
	void Start(FName FirstRow);

	/** Moves to the next line (the current row's NextRow). Returns false when
	 *  the dialogue has ended: the last line was just shown (NextRow empty) or
	 *  the chain broke (missing next row - warn and end rather than blocking
	 *  the player on a dead line). */
	bool Advance();

	/** The row currently on screen; null once the dialogue has ended. */
	const FSnDialogueRow* GetCurrentRow() const { return CurrentRow; }

	/** The current line's portrait, resolved on demand (null when the row has
	 *  none). Called once per line shown, never per frame. */
	UTexture2D* GetPortrait() const;

private:
	/** Caches RowName's row from the global dialogue table (missing rows clear
	 *  the current line with a warning - a broken chain ends the dialogue). */
	void ResolveRow(FName RowName);

	const FSnDialogueRow* CurrentRow = nullptr;
};
