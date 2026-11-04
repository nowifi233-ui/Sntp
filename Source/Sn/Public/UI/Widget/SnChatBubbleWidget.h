// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SnChatBubbleWidget.generated.h"

class UTextBlock;

/**
 * C++ base for the overhead chat bubble (WBP_ChatBubble), shown by the
 * speaking character's WidgetComponent. The blueprint owns the look (frame,
 * tail, fade-in animation); the C++ side only owns the text and the visibility
 * switch, because those are the two things the character drives per message.
 *
 * Not replicated, and it does not need to be: the component exists on every
 * machine with the class, and the message arrives over the multicast - see
 * ASnCharacter::MulticastShowChatBubble. One source (the network) writes this
 * widget, so there is no UI state to race against replication.
 */
UCLASS(Abstract, Blueprintable)
class SN_API USnChatBubbleWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Puts a line on the bubble and shows it. Called per message on every
	 *  machine - the widget itself is never replicated. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void ShowMessage(const FText& Message);

	/** Hides the bubble (the character's timer calls this). The instance stays
	 *  alive - the component owns it for the pawn's whole life. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void HideMessage();

protected:
	/** BP hook: play the pop-in animation. Fired every message, including one
	 *  that replaces the line already on screen. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chat")
	void OnMessageShown();

	/** BP hook: play the fade-out (the widget is already Collapsed by the time
	 *  this returns, so keep it to short sounds/parameter tweaks or drive the
	 *  fade from a separate always-visible wrapper). */
	UFUNCTION(BlueprintImplementableEvent, Category = "Chat")
	void OnMessageHidden();

	/** The line. A plain TextBlock on purpose: the message is a raw string from
	 *  another player and must never be parsed as markup (no rich text here). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BubbleText;
};
