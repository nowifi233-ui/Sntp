// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnAttackCooldownWidget.generated.h"

class UImage;
class UMaterialInstanceDynamic;

/**
 * Ring cooldown indicator at the screen center. Driven by the combat widget
 * controller's OnAttackCooldownChanged: while a cooldown runs, each tick writes
 * the elapsed fraction (0 -> 1, ring fills up) into the ring material's
 * Progress parameter; when it expires the ring collapses and stops ticking.
 */
UCLASS()
class SN_API USnAttackCooldownWidget : public USnUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	virtual void OnWidgetControllerSet() override;

	/** Ring image; the material's Progress parameter drives the sweep. */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> CooldownRing;

	/** Material scalar parameter written each frame with the remaining fraction. */
	UPROPERTY(EditDefaultsOnly, Category = "Cooldown")
	FName RingProgressParameterName = TEXT("Progress");

private:
	UFUNCTION()
	void HandleAttackCooldownChanged(float CooldownSeconds);

	/** World time at which the current cooldown ends. */
	float CooldownEndTime = 0.f;

	/** Duration of the current cooldown in seconds (from the start broadcast). */
	float CooldownDuration = 0.f;

	/** True while the ring is animating; finished rings tick a no-op. */
	bool bActive = false;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial;
};
