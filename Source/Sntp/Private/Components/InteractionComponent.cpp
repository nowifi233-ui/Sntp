// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"

#include "ViewportInteractionTypes.h"
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
		0.2f,
		true);
}

void UInteractionComponent::FindInteractable()
{
	TArray<AActor*> Actors;
	
	UpdateCurrentInteractable();
}

void Update()
{

}

void UInteractionComponent::Interact(int32 Index)
{
	OnOptionSelected.Broadcast(Index);
	if (!CurrentOptions.IsValidIndex(Index))
	{
		return;
	}
	const FInteractionOption& Option = CurrentOptions[Index];
	if (!Option.SourceActor.Get()) return;
	IInteractable* Interactable = Cast<IInteractable>(Option.SourceActor);
	if (Interactable)
	{
		Interactable->Interact(GetOwner(), Option.OptionName);
	}
}

void UInteractionComponent::InitSphere(USphereComponent* InSphereComponent)
{
	DetectionSphere = InSphereComponent;
	if (!DetectionSphere)
	{
		return;
	}
	DetectionSphere->OnComponentBeginOverlap.AddDynamic(this, &UInteractionComponent::OnBeginOverlap);
	DetectionSphere->OnComponentEndOverlap.AddDynamic(this, &UInteractionComponent::OnEndOverlap);
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
	ShowWidgetDelegate.Broadcast(Interactable->GetInteractionOptions(), Interactable->GetInteractionName());
	Interactable->StateChangeDelegate.Clear();
	Interactable->StateChangeDelegate.AddDynamic(this, &ThisClass::CurrentActorStateChanged);
}

void UInteractionComponent::UpdateCurrentInteractable()
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
	
	AActor* Closest = nullptr;
	float BestDist = MAX_FLT;
	
	
	for (AActor* Actor : Actors)
	{
		if (!Actor->Implements<UInteractable>()) continue;
		const float Dist = FVector::Dist(GetOwner()->GetActorLocation(), Actor->GetActorLocation());
		if (Dist < BestDist)
		{
			Closest = Actor;
			BestDist = Dist;
		}
	}
	
	
	if (!Closest)
	{
		CurrentOptions.Empty();
		SetCurrentInteractable(nullptr);
		HideWidgetDelegate.Broadcast();
		return;
	}
	
	IInteractable* ClosestInteractable = Cast<IInteractable>(Closest);
	TArray<FInteractionOption> Options;
	
	if (ClosestInteractable->GetType() == EInteractionType::Building)
	{
		Options = ClosestInteractable->GetInteractionOptions();
		SetCurrentInteractable(Closest);
	}
	else if (ClosestInteractable->GetType() == EInteractionType::Pickup)
	{
		for (AActor* Actor : Actors)
		{
			if (!Actor->Implements<UInteractable>()) continue;
			IInteractable* Interactable = Cast<IInteractable>(Actor);
			
			if (Interactable->GetType() == EInteractionType::Pickup)
			{
				Options.Add(Interactable->GetInteractionOptions()[0]);
			}
		}
	}
	/*
	 * Do Nothing
	 */
	AActor* LastClosestActor = CurrentInteractable;
	bool bSameActor = (Closest == LastClosestActor);
	bool bSameOptions = AreOptionsEqual(CurrentOptions, Options);
	if (bSameActor && bSameOptions)
	{
		return;
	}
	CurrentInteractable = Closest;
	CurrentOptions = Options;
	
	if (CurrentOptions.Num() == 0)
	{
		HideWidgetDelegate.Broadcast();
	}
	else
	{
		ShowWidgetDelegate.Broadcast(CurrentOptions, ClosestInteractable->GetInteractionName());
	}
}

void UInteractionComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UInteractable>())
	{
		return;
	}
	UpdateCurrentInteractable();
}

void UInteractionComponent::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	UpdateCurrentInteractable();
}

void UInteractionComponent::CurrentActorStateChanged()
{
	if (!CurrentInteractable)
	{
		HideWidgetDelegate.Broadcast();
		return;
	}
	
	IInteractable* Interactable = Cast<IInteractable>(CurrentInteractable);
	CurrentOptions = Interactable->GetInteractionOptions();
	ShowWidgetDelegate.Broadcast(Interactable->GetInteractionOptions(), Interactable->GetInteractionName());
}

bool UInteractionComponent::AreOptionsEqual(const TArray<FInteractionOption>& A, const TArray<FInteractionOption>& B)
{
	if (A.Num() != B.Num()) return false;

	for (int32 i = 0; i < A.Num(); ++i)
	{
		if (A[i].SourceActor != B[i].SourceActor)
		{
			return false;
		}
	}
	return true;
}







