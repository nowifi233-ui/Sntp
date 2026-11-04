// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SnLoadingScreen.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

namespace
{
	/** Near-black with a hint of blue. Full-bleed and flat on purpose: the cut
	 *  from gameplay to loading should read as a screen, not as a leaked frame. */
	const FLinearColor LoadingBackgroundColor(0.02f, 0.03f, 0.05f, 1.f);
	const FLinearColor LoadingTitleColor(1.f, 1.f, 1.f, 1.f);
	const FLinearColor LoadingTextColor(0.72f, 0.76f, 0.85f, 1.f);

	/**
	 * Title over a spinner over a status line, centered on a full-bleed fill.
	 *
	 * Everything here is ticked and painted on the movie player's Slate loading
	 * thread, so the widget owns plain values and Slate widgets only: no
	 * UObjects, no delegates back into game code, no game state read at tick
	 * time (all of that is game-thread bound, and reading it from here is a
	 * race). The animation is SThrobber's own FCurveSequence, which advances off
	 * Slate time - and the loading thread ticks the Slate application itself, so
	 * the dots keep moving while the game thread is blocked.
	 */
	class SSnLoadingScreen : public SCompoundWidget
	{
	public:
		SLATE_BEGIN_ARGS(SSnLoadingScreen)
			: _TitleText(FText::GetEmpty())
			, _LoadingText(FText::GetEmpty())
			{}
			/** Game name, large, on top. */
			SLATE_ARGUMENT(FText, TitleText)
			/** Status line under the spinner ("Loading..."). */
			SLATE_ARGUMENT(FText, LoadingText)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs)
		{
			ChildSlot
			[
				SNew(SBorder)
				.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
				.BorderBackgroundColor(FSlateColor(LoadingBackgroundColor))
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					[
						SNew(STextBlock)
						.Text(InArgs._TitleText)
						.ColorAndOpacity(FSlateColor(LoadingTitleColor))
						.Font(FCoreStyle::GetDefaultFontStyle("Bold", 48))
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 28.f, 0.f, 0.f)
					[
						SNew(SThrobber)
					]
					+ SVerticalBox::Slot()
					.AutoHeight()
					.HAlign(HAlign_Center)
					.Padding(0.f, 18.f, 0.f, 0.f)
					[
						SNew(STextBlock)
						.Text(InArgs._LoadingText)
						.ColorAndOpacity(FSlateColor(LoadingTextColor))
						.Font(FCoreStyle::GetDefaultFontStyle("Regular", 20))
					]
				]
			];
		}
	};
}

TSharedRef<SWidget> SnMakeLoadingScreenWidget(const FText& TitleText, const FText& LoadingText)
{
	// The texts are copied into the widget here, on the game thread, so the
	// loading thread only ever touches values that never change again.
	return SNew(SSnLoadingScreen)
		.TitleText(TitleText)
		.LoadingText(LoadingText);
}
