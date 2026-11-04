// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/SnWaveBannerWidget.h"
#include "Components/TextBlock.h"
#include "Spawn/SnSurvivalSpawner.h"

void USnWaveBannerWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Never visible until a wave actually starts.
	SetVisibility(ESlateVisibility::Collapsed);
}

void USnWaveBannerWidget::OnWidgetControllerSet()
{
	Super::OnWidgetControllerSet();

	if (ASnSurvivalSpawner* Spawner = Cast<ASnSurvivalSpawner>(WidgetController))
	{
		Spawner->OnWaveChanged.AddDynamic(this, &USnWaveBannerWidget::HandleWaveChanged);

		// Pull the state we just bound to: a plain actor data source has no
		// BroadcastInitialValues, so a wave that started before this widget
		// existed (bAutoStart - the HUD is built by the ability-system ready
		// chain, well after world BeginPlay) would never be announced. Same
		// reason a rebuilt HUD would otherwise skip the current wave.
		if (Spawner->IsRunning())
		{
			HandleWaveChanged(Spawner->GetDifficultyWave());
		}
	}
}

void USnWaveBannerWidget::HandleWaveChanged(int32 Wave)
{
	ShowWave(Wave);
}

void USnWaveBannerWidget::ShowWave(int32 Wave)
{
	if (WaveText)
	{
		WaveText->SetText(FText::Format(WaveTextFormat, Wave));
	}

	// Restart the timeline (a wave mid-animation simply replaces the current one).
	TimelineElapsed = 0.f;
	bActive = true;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	SetRenderOpacity(0.f);
}

void USnWaveBannerWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Idle (collapsed) widgets tick a no-op.
	if (!bActive)
	{
		return;
	}

	TimelineElapsed += InDeltaTime;

	// Fade in (0 -> 1), hold at 1, then fade out (1 -> 0).
	const float FadeInEnd = FadeInDuration;
	const float FadeOutStart = FadeInDuration + HoldDuration;
	const float Opacity = TimelineElapsed < FadeInEnd
		? TimelineElapsed / FMath::Max(FadeInDuration, KINDA_SMALL_NUMBER)
		: TimelineElapsed < FadeOutStart
			? 1.f
			: 1.f - (TimelineElapsed - FadeOutStart) / FMath::Max(FadeOutDuration, KINDA_SMALL_NUMBER);

	SetRenderOpacity(FMath::Clamp(Opacity, 0.f, 1.f));

	if (TimelineElapsed >= FadeOutStart + FadeOutDuration)
	{
		bActive = false;
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
