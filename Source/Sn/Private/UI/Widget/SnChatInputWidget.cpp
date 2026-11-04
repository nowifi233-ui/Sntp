// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnChatInputWidget.h"
#include "Components/EditableTextBox.h"
#include "InputCoreTypes.h"
#include "Player/SnPlayerController.h"
#include "Widgets/SWidget.h"

void USnChatInputWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// AddUnique: resident widget, so there is only ever one construct - but the
	// delegate is the one thing that would double-fire a message if that ever
	// stopped being true.
	if (ChatInputBox)
	{
		ChatInputBox->OnTextCommitted.AddUniqueDynamic(this, &USnChatInputWidget::HandleTextCommitted);

		// Hand the keyboard back on commit: the line is over the moment Enter
		// (or a click away) lands, and a hidden widget holding focus is the
		// classic source of "the next Escape does nothing".
		ChatInputBox->SetClearKeyboardFocusOnCommit(true);
	}
}

void USnChatInputWidget::Activate()
{
	if (!ChatInputBox)
	{
		return;
	}

	// The instance is resident, so the previous line - or the one a cancelled
	// open left behind - must not survive into this one.
	ChatInputBox->SetText(FText::GetEmpty());
	ChatInputBox->SetKeyboardFocus();
}

void USnChatInputWidget::Deactivate()
{
	if (ChatInputBox)
	{
		ChatInputBox->SetText(FText::GetEmpty());
	}
}

TSharedPtr<SWidget> USnChatInputWidget::GetFocusWidget() const
{
	// TakeWidget, not GetCachedWidget: on the first open the Slate side may not
	// exist yet, and TakeWidget builds it (the same call AddToViewport makes).
	// Guarded rather than a ternary: TakeWidget returns a TSharedRef, so a null
	// branch would have to convert to a non-nullable type and fails to compile.
	if (!ChatInputBox)
	{
		return nullptr;
	}
	return ChatInputBox->TakeWidget();
}

FReply USnChatInputWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Held keys repeat from the OS: swallow the repeats so a held Escape closes
	// the line once (same guard as USnActivatableWindow's key handling).
	if (InKeyEvent.IsRepeat())
	{
		return FReply::Handled();
	}

	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		// Cancel: close without sending. Empty Enter does the same thing one
		// level down (SubmitChatText drops an empty line), so the two exits
		// converge on the same state.
		if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetOwningPlayer()))
		{
			SnPC->CloseChatInput();
		}
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void USnChatInputWidget::HandleTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod != ETextCommit::OnEnter)
	{
		return;
	}

	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetOwningPlayer()))
	{
		// ToString() copies before the call: the close path clears the box, and
		// this reference points into the box's own text.
		SnPC->SubmitChatText(Text.ToString());
	}
}
