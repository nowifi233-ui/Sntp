// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/InteractableBuildingBase.h"

TArray<FInteractionOption> AInteractableBuildingBase::GetInteractionOptions()
{
	for (auto& Option : Options)
	{
		Option.SourceActor = this;
	}
	return Options;
}

void AInteractableBuildingBase::Interact(AActor* Interactor, FName OptionName)
{
	Blueprint_Interact(Interactor, OptionName);
}
