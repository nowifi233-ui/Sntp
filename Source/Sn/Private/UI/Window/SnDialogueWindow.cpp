// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Window/SnDialogueWindow.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Dialogue/FSnDialogueRow.h"
#include "Dialogue/USnDialogueController.h"
#include "Game/SnSettings.h"
#include "InputCoreTypes.h"
#include "Player/SnPlayerController.h"

FReply USnDialogueWindow::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Click anywhere in the window drives the dialogue (interactive children
	// like buttons consume their own clicks and never advance). Unlike the
	// base window there is no raise to do here - a dialogue always opens as
	// the topmost window and nothing can open above it while it is up.
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (USnDialogueController* Controller = GetController())
		{
			// While the line is still typing itself out, the click completes it
			// instead of advancing. Two reasons: a fast reader never has to wait
			// for the animation, and the click cannot eat a line that just
			// appeared (the classic double-click-loses-a-line bug).
			if (bRevealing)
			{
				CompleteReveal();
			}
			else if (Controller->Advance())
			{
				ShowCurrentLine();
			}
			else
			{
				// The last line was shown: the dialogue is over - close like
				// Escape would (the same path also covers a broken chain).
				if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetOwningPlayer()))
				{
					SnPC->CloseWindow();
				}
			}
			return FReply::Handled();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void USnDialogueWindow::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// One branch per frame while idle. Also covers a window closed mid-reveal
	// with Escape: the next PlayDialogue injects a NEW controller, which fires
	// OnWidgetControllerSet -> ShowCurrentLine -> a fresh reveal, so a stale
	// half-revealed line can never leak into the next dialogue.
	if (!bRevealing)
	{
		return;
	}

	// Slate time, not world time: the tutorial popup pauses the game
	// (PlayDialogue(bPauseGame=true)) and the line has to keep typing behind the
	// pause. Widget ticks are driven by the Slate tick, which the engine runs
	// every frame regardless of SetGamePaused. A world timer would freeze here.
	RevealElapsed += InDeltaTime;

	// The target comes from ELAPSED time, never from one character per tick:
	// that is what makes the speed independent of the frame rate (and lets the
	// reveal catch up by itself after a hitch or a long first frame).
	const int32 TargetCount = FMath::FloorToInt(RevealElapsed * USnSettings::Get()->GetDialogueCharsPerSecond());

	bool bChanged = false;
	while (bRevealing && RevealedCount < TargetCount)
	{
		if (!AdvanceOneCodePoint())
		{
			break;
		}
		bChanged = true;
	}

	// Only when the visible length actually changed: every SetText re-runs the
	// text layout, and most frames add no character at all.
	if (bChanged)
	{
		UpdateRevealedText();
	}
}

void USnDialogueWindow::OnWidgetControllerSet()
{
	Super::OnWidgetControllerSet();
	ShowCurrentLine();
}

void USnDialogueWindow::ShowCurrentLine()
{
	USnDialogueController* Controller = GetController();
	const FSnDialogueRow* Row = Controller ? Controller->GetCurrentRow() : nullptr;

	// Speaker: narration lines hide it so the text reads centered.
	if (Row && !Row->SpeakerName.IsEmpty())
	{
		SpeakerNameText->SetText(Row->SpeakerName);
		SpeakerNameText->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		SpeakerNameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Portrait: rows without one hide the image (the box stays full width).
	if (UTexture2D* Portrait = Controller ? Controller->GetPortrait() : nullptr)
	{
		PortraitImage->SetBrushFromTexture(Portrait);
		PortraitImage->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		PortraitImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	StartReveal(Row ? Row->LineText : FText::GetEmpty());
}

void USnDialogueWindow::StartReveal(const FText& LineText)
{
	// Resolved once per line: the reveal slices this string every step, so it
	// must not re-resolve the row (or re-run ToString) per frame. ToString is
	// the culture-correct display string - the same text the block showed before
	// the typewriter existed.
	FullLine = LineText.ToString();
	RevealedCodeUnits = 0;
	RevealedCount = 0;
	RevealElapsed = 0.f;

	if (FullLine.IsEmpty() || USnSettings::Get()->GetDialogueCharsPerSecond() <= 0.f)
	{
		// Empty line, or the typewriter switched off in Sn Settings (0 = off).
		CompleteReveal();
		return;
	}

	bRevealing = true;

	// First character immediately, not after 1/speed seconds - otherwise the
	// popup sits blank for the first frames (2 frames at 30 chars/s) and reads
	// as a hitch rather than as typing.
	AdvanceOneCodePoint();
	UpdateRevealedText();
}

bool USnDialogueWindow::AdvanceOneCodePoint()
{
	if (RevealedCodeUnits >= FullLine.Len())
	{
		// Nothing left to reveal (only reachable from a stale target in the tick -
		// the last character of a line always clears bRevealing below).
		return false;
	}

	// Whole code points only. FString is UTF-16, so cutting a surrogate pair in
	// half renders a replacement glyph for a frame - visible on emoji and the
	// rare CJK extensions. A pair counts as ONE revealed character.
	const bool bSurrogatePair = FullLine.IsValidIndex(RevealedCodeUnits + 1)
		&& StringConv::IsHighSurrogate(static_cast<uint32>(FullLine[RevealedCodeUnits]))
		&& StringConv::IsLowSurrogate(static_cast<uint32>(FullLine[RevealedCodeUnits + 1]));

	RevealedCodeUnits += bSurrogatePair ? 2 : 1;
	++RevealedCount;

	// The line is complete the moment its last code point lands - do not wait
	// for the next target step to notice. In that window a click would still
	// read as "complete the line" and get swallowed instead of advancing.
	bRevealing = RevealedCodeUnits < FullLine.Len();
	return true;
}

void USnDialogueWindow::CompleteReveal()
{
	bRevealing = false;
	RevealedCodeUnits = FullLine.Len();
	UpdateRevealedText();
}

void USnDialogueWindow::UpdateRevealedText()
{
	LineTextBlock->SetText(FText::FromString(FullLine.Left(RevealedCodeUnits)));
}

USnDialogueController* USnDialogueWindow::GetController() const
{
	return Cast<USnDialogueController>(GetWidgetController());
}
