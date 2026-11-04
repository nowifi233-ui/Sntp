// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/SnDamageNumber.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

void USnDamageNumber::ShowAt(const FVector& WorldLocation, float Amount, const FName InStyleTag, bool bPrefixPlus, bool bAppendBang, const FString& InTextOverride, int32 ScatterIndex)
{
	// Project to screen (the local player's view - multiplayer shows each
	// client's own numbers).
	FVector2D ScreenPos = FVector2D::ZeroVector;
	if (APlayerController* PC = GetOwningPlayer())
	{
		UGameplayStatics::ProjectWorldToScreen(PC, WorldLocation, ScreenPos, /*bPlayerViewportRelative=*/true);
	}

	// Deterministic ring scatter (golden angle): consecutive numbers - e.g.
	// one attack's physical + enchant hits, or an AOE hitting several targets
	// in the same frame - land on different ring angles and radii, so they
	// never stack on the same screen position.
	const float GoldenAngle = 2.399963f; // 137.5 degrees
	const float Angle = ScatterIndex * GoldenAngle;
	const float Radius = ScatterBaseRadius + (ScatterIndex % 4) * ScatterRadiusStep;
	SetPositionInViewport(ScreenPos + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);

	// Pool-reuse reset: the previous show's animation left a translation and a
	// fade behind. The C++ side owns the placement, the blueprint animation
	// owns everything after it - so reset both before the animation replays.
	SetRenderTransform(FWidgetTransform());
	SetRenderOpacity(1.f);

	// Stop any animation the previous show may still be playing: with pooled
	// reuse, a ShowAt can land mid-animation (rapid hits) - the leftover
	// sequence player would keep writing RenderTransform/Opacity over this
	// show's values until the blueprint's PlayAnimation restarts it. Kill it
	// first, so the new animation starts from a clean slate.
	StopAllAnimations();

	if (NumberText)
	{
		// Rich text: the style tag names a row in the widget's TextStyleSet
		// (e.g. "Damage.Lightning" for the purple lightning number). A missing
		// row falls back to the style set's DefaultTextStyle, so an unconfigured
		// table degrades to plain text instead of vanishing.
		FString Content;
		if (!InTextOverride.IsEmpty())
		{
			Content = InTextOverride;
		}
		else
		{
			const int32 DisplayValue = FMath::RoundToInt(Amount);
			Content = FString::Printf(TEXT("%s%d%s"), bPrefixPlus ? TEXT("+") : TEXT(""), DisplayValue, bAppendBang ? TEXT("!") : TEXT(""));
		}
		NumberText->SetText(FText::FromString(FString::Printf(TEXT("<%s>%s</>"), *InStyleTag.ToString(), *Content)));
	}
	SetVisibility(ESlateVisibility::Visible);

	// Fallback exit: if the blueprint animation never calls FinishNumber (an
	// unconfigured WBP), force the pool return so the pool cannot stay pinned
	// open by a broken instance.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(FinishTimerHandle, this, &USnDamageNumber::HandleTimeout, MaxLifetime, /*bLoop=*/false);
	}

	// Number is fully set up - let the bound float animation play. Fires for
	// pooled reuses too: the resident instance's animation restarts from its
	// beginning each time.
	OnShown.Broadcast();
}

void USnDamageNumber::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// Must stay ticking: the 5.6+ animation tick manager disables animations on
	// widgets that are not ticking (it checks bIsTicking at Slate's PreTick,
	// before this widget's own Slate tick can report itself via OnWidgetTicked).
	// Without a tick source, a PlayAnimation is killed one frame later -
	// DisableAnimations() fires the finish callback instantly, collapsing the
	// number before it is ever seen. The Super call reports OnWidgetTicked; the
	// body is empty on purpose - motion and fade live in the blueprint
	// animation.
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void USnDamageNumber::FinishNumber()
{
	// Only exit path (besides the timeout): the float animation ended -
	// collapse the number and return it to the pool.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FinishTimerHandle);
	}
	SetVisibility(ESlateVisibility::Collapsed);
	OnFinished.ExecuteIfBound();
}

void USnDamageNumber::HandleTimeout()
{
	// The animation never finished (unconfigured WBP): take the same exit path
	// as a finished animation, so the pool always recycles.
	FinishNumber();
}
