// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/InventoryActorBase.h"

#include "Characters/SntpPlayerCharacter.h"
#include "UI/SntpHUD.h"

AInventoryActorBase::AInventoryActorBase()
{
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(FName("InventoryComponent"));
	InventoryComponent->Capacity = 100;
	InventoryComponent->InitInventory();
}

TArray<FInteractionOption> AInventoryActorBase::GetInteractionOptions()
{
	for (auto& Option : Options)
	{
		Option.SourceActor = this;
	}
	return Options;
}

void AInventoryActorBase::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == FName("Open"))
	{
		// Show Widget
		// Interactor->Player->Controller->HUD->ShowWidget
		// Widget->WidgetController
		APlayerController* PC = Cast<APlayerController>(Cast<ASntpPlayerCharacter>(Interactor)->GetController());
		ASntpHUD* SntpHUD = Cast<ASntpHUD>(PC->GetHUD());
		if (SntpHUD)
		{
			SntpHUD->ToggleInventoryUI(PC, InventoryComponent);
		}
	}
	if (OptionName == FName("Recall"))
	{
		Destroy();
	}
}
