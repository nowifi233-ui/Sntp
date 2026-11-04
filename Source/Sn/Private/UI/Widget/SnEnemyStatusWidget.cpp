// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnEnemyStatusWidget.h"

void USnEnemyStatusWidget::UpdateHealthPercent(float Percent)
{
	TargetPercent = FMath::Clamp(Percent, 0.f, 1.f);
	OnHealthPercentChanged.Broadcast(TargetPercent);
}
