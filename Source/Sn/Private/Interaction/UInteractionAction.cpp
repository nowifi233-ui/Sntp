// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/UInteractionAction.h"
#include "Player/SnPlayerController.h"

void UInteractionAction::Execute_Implementation(APlayerController* PC, AActor* Target)
{
	// Base implementation is intentionally a warning: subclasses must override.
	UE_LOG(LogTemp, Warning, TEXT("UInteractionAction: %s has no Execute override"), *GetName());
}

void UInteractionAction::OpenConfigWindow(APlayerController* PC, UObject* DataSource)
{
	// Table-driven: the widget class comes from the interaction row, not from
	// the action blueprint (nothing hardcoded).
	if (!ConfigWidgetClass)
	{
		return;
	}
	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(PC))
	{
		SnPC->OpenConfigWindow(ConfigWidgetClass, DataSource);
	}
}
