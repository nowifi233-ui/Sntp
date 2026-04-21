// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Pickups/PickupActor.h"

#include "Components/InventoryComponent.h"

// Sets default values
APickupActor::APickupActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

APickupActor::APickupActor(UItemDefinition* InItemDef, int32 InCount)
{
	ItemDef	= InItemDef;
	ItemName = *InItemDef->Name.ToString();
	Count = InCount;
}

TArray<FInteractionOption> APickupActor::GetInteractionOptions()
{
	TArray<FInteractionOption> Options;
	if (!ItemDef)
	{
		return Options;
	}
	FInteractionOption Option;
	Option.OptionName = FName("Pickup");
	Option.DisplayText = ItemDef->Name;
	Option.Icon = ItemDef->Icon;
	Option.Type = EInteractionType::Pickup;
	Option.Count = Count;
	Option.SourceActor = this;
	Options.Add(Option);
	return Options;
}

void APickupActor::Interact(AActor* Interactor, FName OptionName)
{
	UInventoryComponent* Inv = Interactor->FindComponentByClass<UInventoryComponent>();
	if (Inv && ItemDef)
	{
		Inv->AddItem(ItemDef, Count);
		Destroy();
	}
}

void APickupActor::SetItem(UItemDefinition* InItemDef, int32 InCount)
{
	ItemDef	= InItemDef;
	ItemName = *InItemDef->Name.ToString();
	Count = InCount;
}


