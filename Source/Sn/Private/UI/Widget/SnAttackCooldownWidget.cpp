// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widget/SnAttackCooldownWidget.h"
#include "Components/Image.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/WidgetController/SnCombatWidgetController.h"

void USnAttackCooldownWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Cache the dynamic instance so the tick never re-creates it.
	if (CooldownRing)
	{
		DynamicMaterial = CooldownRing->GetDynamicMaterial();
	}

	// Never visible until a cooldown actually starts.
	SetVisibility(ESlateVisibility::Collapsed);
}

void USnAttackCooldownWidget::OnWidgetControllerSet()
{
	Super::OnWidgetControllerSet();

	if (USnCombatWidgetController* CombatController = Cast<USnCombatWidgetController>(WidgetController))
	{
		CombatController->OnAttackCooldownChanged.AddDynamic(this, &USnAttackCooldownWidget::HandleAttackCooldownChanged);
	}
}

void USnAttackCooldownWidget::HandleAttackCooldownChanged(float CooldownSeconds)
{
	if (CooldownSeconds > 0.f)
	{
		// Cooldown started: animate from full to empty over the reported duration.
		CooldownDuration = CooldownSeconds;
		CooldownEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + CooldownSeconds : 0.f;
		bActive = true;
		SetVisibility(ESlateVisibility::Visible);
	}
	else if (bActive)
	{
		// Cooldown ended: hide now instead of waiting for the tick to catch up.
		bActive = false;
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USnAttackCooldownWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Inactive (pooled/hidden) widgets tick a no-op.
	if (!bActive || !DynamicMaterial)
	{
		return;
	}

	const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	// Elapsed fraction 0 -> 1: the ring fills up as the cooldown runs out.
	const float Fraction = FMath::Clamp(1.f - (CooldownEndTime - Now) / FMath::Max(CooldownDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
	DynamicMaterial->SetScalarParameterValue(RingProgressParameterName, Fraction);

	if (Fraction >= 1.f)
	{
		bActive = false;
		SetVisibility(ESlateVisibility::Collapsed);
	}
}
