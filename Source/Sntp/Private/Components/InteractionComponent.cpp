// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/InteractionComponent.h"

#include "Components/CapsuleComponent.h"
#include "Interaction/Interactable.h"
#include "Kismet/KismetSystemLibrary.h"

class UCapsuleComponent;
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
		&UInteractionComponent::UpdateCurrentInteractable,
		0.2f,
		true);
}

void UInteractionComponent::Interact(FInteractionOption Option)
{
	OnOptionSelected.Broadcast(Option);

	if (!Option.SourceActor.Get()) return;
	IInteractable* Interactable = Cast<IInteractable>(Option.SourceActor);
	if (Interactable)
	{
		Interactable->Interact(GetOwner(), Option.OptionName);
	}
}

void UInteractionComponent::InitSphere(UCapsuleComponent* InSphereComponent)
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
	
	UKismetSystemLibrary::CapsuleOverlapActors(
		GetWorld(),
		GetOwner()->GetActorLocation(),
		150,
		180,
		TArray<TEnumAsByte<EObjectTypeQuery>>(),
		nullptr,
		{},
		Actors);
	
	// UKismetSystemLibrary::DrawDebugCapsule(GetWorld(), GetOwner()->GetActorLocation(), 180, 150, FRotator(), FColor::Blue);
	
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

	AActor* LastClosestActor = CurrentInteractable;
	bool bSameActor = (Closest == LastClosestActor);
	bool bSameOptions = AreOptionsEqual(CurrentOptions, Options);
	if (bSameActor && bSameOptions)
	{
		return;
	}
	
	CurrentInteractable = Closest;
	CurrentOptions = Options;
	
	if (CurrentInteractable && LastClosestActor)
	{
		if (CurrentInteractable->Implements<UInteractable>() && LastClosestActor->Implements<UInteractable>())
		{
			if (Cast<IInteractable>(LastClosestActor)->GetType() == EInteractionType::Building && Cast<IInteractable>(CurrentInteractable)->GetType() == EInteractionType::Pickup)
			{
				// ShowWidgetDelegate.Broadcast(CurrentOptions, Cast<IInteractable>(CurrentInteractable)->GetInteractionName());
				HideWidgetDelegate.Broadcast();
				for (const FInteractionOption& Option : CurrentOptions)
				{
					AddOptionDelegate.Broadcast(Option);
				}
			}
		}
	}
	if (CurrentOptions.Num() == 0)
	{
		HideWidgetDelegate.Broadcast();
	}
}

void UInteractionComponent::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor->Implements<UInteractable>())
	{
		return;
	}
	IInteractable* ClosestInteractable = Cast<IInteractable>(OtherActor);
	if (ClosestInteractable->GetType() == EInteractionType::Pickup)
	{
		FInteractionOption Option = ClosestInteractable->GetInteractionOptions()[0];
		if (CurrentInteractable && CurrentInteractable->Implements<UInteractable>() && Cast<IInteractable>(CurrentInteractable)->GetType() == EInteractionType::Building)
		{
			HideWidgetDelegate.Broadcast();
		}
		AddOptionDelegate.Broadcast(Option);
	}
	else
	{
		UpdateCurrentInteractable();
	}
}

void UInteractionComponent::OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor->Implements<UInteractable>())
	{
		return;
	}
	IInteractable* ClosestInteractable = Cast<IInteractable>(OtherActor);
	if (ClosestInteractable->GetType() == EInteractionType::Pickup)
	{
		FInteractionOption Option = ClosestInteractable->GetInteractionOptions()[0];
		RemoveOptionDelegate.Broadcast(Option);
	}
	else
	{
		UpdateCurrentInteractable();
	}
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







