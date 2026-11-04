// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SnChatInputWidget.generated.h"

class SWidget;
class UEditableTextBox;

/**
 * C++ base for the chat input line (WBP_ChatInput), opened by Enter on the
 * player controller. The blueprint owns the look (frame, position); C++ owns
 * the three behaviours that must not be mis-wired: Enter commits the line,
 * Escape cancels it, and the box starts empty and focused on every open.
 *
 * Deliberately NOT a USnActivatableWindow: this is a typing line, not a menu.
 * It shows no cursor, counts as no "config window" (so the game's Escape
 * fallback stays out of it - Escape here is a Slate key event), and takes no
 * Z slot in the window manager. The one thing it does share is the input mode:
 * UI-only, so typing cannot drive the character - see
 * USnWindowManager::ApplyInputModeNow.
 *
 * Resident instance (created once by the HUD, Collapsed when closed): a
 * rebuild per open would need the text box to re-focus against a freshly
 * added Slate widget, which is exactly the timing this project avoids
 * everywhere else.
 */
UCLASS(Abstract, Blueprintable)
class SN_API USnChatInputWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens the line for typing: empty box, keyboard focus. Called by the HUD
	 *  after the widget is made visible. */
	void Activate();

	/** Closes the line: drops the text so the resident instance never reopens
	 *  with the last line in it. Keyboard focus is returned by the input mode
	 *  switch (game-only hands it back to the viewport). */
	void Deactivate();

	/** The Slate widget the input mode should focus - the box itself. Focusing
	 *  the user widget would leave the keystrokes unhandled: the box is what
	 *  consumes typing. */
	TSharedPtr<SWidget> GetFocusWidget() const;

protected:
	virtual void NativeConstruct() override;

	/** Escape closes the line without sending. Received here (not by a game
	 *  binding) because UI-only input gives Slate the keyboard: the focused
	 *  text box does not consume Escape (it only does while text is selected),
	 *  so the key bubbles up to this widget. */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Enter sends the line; every other commit reason (focus loss, click away)
	 *  is ignored - a stolen focus must not fire a half-typed message. */
	UFUNCTION()
	void HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** The typing box (single line). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UEditableTextBox> ChatInputBox;
};
