// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/InteractablePoint.h"

// Sets default values
AInteractablePoint::AInteractablePoint()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

TArray<FInteractionOption> AInteractablePoint::GetInteractionOptions()
{
	for (auto& Option : Options)
	{
		Option.SourceActor = this;
	}
	return Options;
}

void AInteractablePoint::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == FName("Interact1"))
	{
		OnInteract1();	
	}
	else if (OptionName == FName("Interact2"))
	{
		OnInteract2();
	}
}

