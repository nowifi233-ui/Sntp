// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Window/SnActivatableWindow.h"
#include "SnDialogueWindow.generated.h"

class UImage;
class UTextBlock;
class USnDialogueController;

/**
 * One-line-at-a-time dialogue popup (the intro tutorial and the tutorial
 * zones are its first users - PlayDialogue on the PC opens it). A regular
 * resident window: clicking anywhere advances the line, the last line closes
 * the window, and Escape (the base window's key handling) skips the whole
 * dialogue.
 *
 * Each line TYPES ITSELF OUT (typewriter / 逐字显示) at
 * USnSettings::DialogueCharsPerSecond, and a click while it is still typing
 * completes the line instead of advancing - the standard contract, so a fast
 * reader is never blocked by the animation and never loses a freshly shown
 * line to a double click.
 *
 * The reveal runs on NativeTick, i.e. on Slate time. The tutorial variant of
 * this popup PAUSES the game (PlayDialogue(bPauseGame=true)) and the text has
 * to keep typing behind the pause: widget ticks come from the Slate tick,
 * which the engine runs every frame regardless of SetGamePaused (the same
 * timeline the window animations use). Do NOT move this to a world timer -
 * timers freeze with the pause and the line would stop mid-word.
 *
 * Pausing itself is NOT this class's business: the hub intro opens it while
 * nothing threatens the player, and UI-only input already locks all game
 * controls while it is visible. LAN never plays it (see the tutorial trigger),
 * so there is no co-op pause hazard either - the pause flag is owned by the PC
 * (PlayDialogue / UpdateDialoguePause).
 */
UCLASS()
class SN_API USnDialogueWindow : public USnActivatableWindow
{
	GENERATED_BODY()

protected:
	/** Click anywhere in the window drives the dialogue: mid-reveal it completes
	 *  the line, otherwise it advances to the next one; the click that ends the
	 *  dialogue closes the window (the dialogue is done, see Advance). */
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Advances the typewriter. Idle frames cost one branch (the bRevealing
	 *  guard) - the same discipline as the wave banner and the cooldown ring. */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Shows the first line as soon as the controller is injected. */
	virtual void OnWidgetControllerSet() override;

	/** Speaker name above the line (empty lines hide it - narration). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> SpeakerNameText;

	/** Character portrait (rows without one hide it). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> PortraitImage;

	/** The current line's text. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> LineTextBlock;

private:
	/** Pulls the current row from the controller into the three named widgets
	 *  and starts the typewriter on the new line. The single entry point for "a
	 *  line is on screen": the first line through OnWidgetControllerSet, every
	 *  later one through the click handler. */
	void ShowCurrentLine();

	/** Starts the typewriter on this line: copies the string, shows the first
	 *  character straight away, reveals the rest over time. Shows the whole line
	 *  at once when it is empty or the configured speed is 0 (typewriter off). */
	void StartReveal(const FText& LineText);

	/** Reveals the next code point; returns false when there was nothing left to
	 *  reveal. Clears bRevealing the moment the last character lands - from then
	 *  on a click advances instead of completing. */
	bool AdvanceOneCodePoint();

	/** Puts the whole line on screen at once and stops the reveal (skip click,
	 *  typewriter off, or a line that is already complete). */
	void CompleteReveal();

	/** Pushes the revealed prefix into the text block. Called only when the
	 *  visible length actually changed: every SetText re-runs the text layout,
	 *  so this must never be called per frame. */
	void UpdateRevealedText();

	/** The injected dialogue controller (null before the first injection). */
	USnDialogueController* GetController() const;

	/** The current line as display text, resolved once per line - the reveal
	 *  slices this every step and must not re-resolve the row (or re-run
	 *  FText::ToString) per frame. */
	FString FullLine;

	/** How much of FullLine is on screen, in UTF-16 code units (the slice index
	 *  for FullLine.Left). */
	int32 RevealedCodeUnits = 0;

	/** How many CODE POINTS have been revealed - the pacing counter the tick
	 *  compares its elapsed-time target against. Not derivable from
	 *  RevealedCodeUnits: a surrogate pair is one code point but two code
	 *  units. */
	int32 RevealedCount = 0;

	/** Seconds spent on the current line (Slate time - see NativeTick). */
	float RevealElapsed = 0.f;

	/** True while the current line is still typing itself out. */
	bool bRevealing = false;
};
