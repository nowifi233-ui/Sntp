// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnChatBubbleWidget.h"
#include "Components/TextBlock.h"

void USnChatBubbleWidget::ShowMessage(const FText& Message)
{
	if (BubbleText)
	{
		BubbleText->SetText(Message);
	}

	// Visible, not HitTestInvisible+: the bubble is overhead decoration with no
	// interactions, and the widget as a whole is what the blueprint animates -
	// Collapsed survives into the next message, which is the point of the
	// resident instance (nothing to rebuild, nothing to flicker).
	SetVisibility(ESlateVisibility::Visible);
	OnMessageShown();
}

void USnChatBubbleWidget::HideMessage()
{
	OnMessageHidden();
	SetVisibility(ESlateVisibility::Collapsed);
}
