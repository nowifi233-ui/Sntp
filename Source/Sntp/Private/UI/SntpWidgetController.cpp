// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SntpWidgetController.h"

void USntpWidgetController::SetWidgetControllerParams(const FWidgetControllerParams& Params)
{
	PlayerController = Params.PlayerController;
	PlayerState = Params.PlayerState;
	AbilitySystemComponent = Params.AbilitySystemComponent;
	AttributeSet = Params.AttributeSet;
}
