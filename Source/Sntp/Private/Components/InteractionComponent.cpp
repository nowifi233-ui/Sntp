// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"

#include "Interaction/Interactable.h"
#include "Kismet/KismetSystemLibrary.h"

// Sets default values for this component's properties
UInteractionComponent::UInteractionComponent()
{
	
}

void UInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	GetWorld()->GetTimerManager().SetTimer(
		ScanTimer,
		this, 
		&UInteractionComponent::FindInteractable,
		0.1f,
		true);
}

void UInteractionComponent::FindInteractable()
{
	TArray<AActor*> Actors;
	
	UKismetSystemLibrary::SphereOverlapActors(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		150,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),
		nullptr,
		{},
		Actors);
	
	AActor* Best = nullptr;
	float BestDist = MAX_FLT;
	
	for (AActor* Actor : Actors)
	{
		if (!Actor->Implements<UInteractable>()) continue;
		
		const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Actor->GetActorLocation());
		if (Dist < BestDist)
		{
			Best = Actor;
			BestDist = Dist;
		}
	}
	
	SetCurrentInteractable(Best);
}

void UInteractionComponent::Interact(FName OptionName)
{
	if (!CurrentInteractable)
	{
		return;
	}
	IInteractable* Interactable = Cast<IInteractable>(CurrentInteractable);
	if (Interactable)
	{
		Interactable->Interact(GetOwner(), OptionName);
	}
}

void UInteractionComponent::SetCurrentInteractable(TObjectPtr<AActor> Actor)
{
	if (Actor == CurrentInteractable)
	{
		return;
	}
	CurrentInteractable = Actor;
	if (!CurrentInteractable)
	{
		HideWidgetDelegate.Broadcast();
		return;
	}
	IInteractable* Interactable = Cast<IInteractable>(CurrentInteractable);
	ShowWidgetDelegate.Broadcast(Interactable->GetInteractionOptions());
}







