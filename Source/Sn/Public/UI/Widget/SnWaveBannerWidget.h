// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnWaveBannerWidget.generated.h"

class UTextBlock;

/**
 * Top-center wave banner ("第 N 波"). Resident widget fed by the survival
 * spawner's OnWaveChanged: each wave replays a fade in / hold / fade out
 * timeline driven by NativeTick (same pattern as the cooldown ring and the
 * damage-number pool - bActive short-circuits idle ticks, Collapsed stops
 * ticking entirely). Never torn down; a new wave simply restarts the timeline.
 */
UCLASS()
class SN_API USnWaveBannerWidget : public USnUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Shows the wave text for a beat: fade in, hold, fade out. Restarts the
	 *  timeline if a wave arrives while the banner is still animating. */
	void ShowWave(int32 Wave);

protected:
	virtual void OnWidgetControllerSet() override;

	/** Wave text (layout and styling live in the Blueprint). */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> WaveText;

	/** Text pattern; {0} is replaced with the wave number. */
	UPROPERTY(EditDefaultsOnly, Category = "WaveBanner", meta = (DisplayName = "波次文本格式"))
	FText WaveTextFormat = INVTEXT("第 {0} 波");

	/** Seconds the banner fades in. */
	UPROPERTY(EditDefaultsOnly, Category = "WaveBanner", meta = (DisplayName = "淡入时长（秒）"))
	float FadeInDuration = 0.3f;

	/** Seconds the banner stays fully visible between the fades. */
	UPROPERTY(EditDefaultsOnly, Category = "WaveBanner", meta = (DisplayName = "停留时长（秒）"))
	float HoldDuration = 1.5f;

	/** Seconds the banner fades out. */
	UPROPERTY(EditDefaultsOnly, Category = "WaveBanner", meta = (DisplayName = "淡出时长（秒）"))
	float FadeOutDuration = 0.5f;

private:
	UFUNCTION()
	void HandleWaveChanged(int32 Wave);

	/** Seconds since ShowWave started (drives the fade in/hold/fade out timeline). */
	float TimelineElapsed = 0.f;

	/** True while the banner is animating; idle widgets tick a no-op. */
	bool bActive = false;
};
