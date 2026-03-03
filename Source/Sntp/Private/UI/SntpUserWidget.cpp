// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SntpUserWidget.h"

void USntpUserWidget::SetWidgetController(UObject* Controller)
{
	WidgetController = Controller;
	WidgetControllerSet();
}
