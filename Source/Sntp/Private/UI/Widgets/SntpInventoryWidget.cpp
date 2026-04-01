// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/SntpInventoryWidget.h"

void USntpInventoryWidget::InitComponents(UInventoryComponent* InActorComponent, UInventoryComponent* InPlayerComponent)
{
	ActorInventoryComponent = InActorComponent;
	PlayerInventoryComponent = InPlayerComponent;
}
