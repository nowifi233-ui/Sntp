// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/SntpHUD.h"

void ASntpHUD::BeginPlay()
{
	Super::BeginPlay();
	
	UUserWidget* Widget = CreateWidget<UUserWidget>(GetWorld(), OverlayWidgetClass);
	Widget->AddToViewport();
	
}
