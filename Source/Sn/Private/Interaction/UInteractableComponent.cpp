// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/UInteractableComponent.h"

#include "Net/UnrealNetwork.h"

UInteractableComponent::UInteractableComponent()
{
	// Replicated so bEnabled (pool recycle, toggles) reaches clients on
	// replicated actors - their interaction scan must exclude the same
	// interactables the server excludes.
	SetIsReplicatedByDefault(true);
}

void UInteractableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UInteractableComponent, bEnabled);
}
