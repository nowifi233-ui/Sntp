// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/EquipmentWidgetController.h"

void UEquipmentWidgetController::Init(UEquipmentComponent* InEquipmentComponent, APlayerController* InPlayerController)
{
	PlayerEquipmentComponent = InEquipmentComponent;
	if (InEquipmentComponent)
	{
		PlayerController = InPlayerController;
	}
}
