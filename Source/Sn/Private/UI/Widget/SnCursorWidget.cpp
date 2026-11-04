// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnCursorWidget.h"

void USnCursorWidget::PlayClickEffect()
{
	OnClickEffect();
}

void USnCursorWidget::SetCarriedItem(FName ItemRowName)
{
	OnCarriedItemChanged(ItemRowName);
}

void USnCursorWidget::ClearCarriedItem()
{
	OnCarriedItemChanged(NAME_None);
}
