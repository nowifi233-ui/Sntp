// Fill out your copyright notice in the Description page of Project Settings.

#include "Interaction/UPlayerInteractionComponent.h"
#include "Interaction/UInteractableComponent.h"
#include "TimerManager.h"
#include "Engine/OverlapResult.h"

UPlayerInteractionComponent::UPlayerInteractionComponent()
{
	// Timer-driven scan; no per-frame tick needed.
	PrimaryComponentTick.bCanEverTick = false;
}

void UPlayerInteractionComponent::BeginPlay()
{
	Super::BeginPlay();
	StartScanning();
}

void UPlayerInteractionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ScanTimerHandle.IsValid() && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void UPlayerInteractionComponent::StartScanning()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UPlayerInteractionComponent::Scan, ScanInterval, /*bLoop=*/true);
	}
}

void UPlayerInteractionComponent::ForceScan()
{
	Scan();
}

void UPlayerInteractionComponent::Scan()
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	// Overlap everything in range, then keep only actors that carry an
	// interactable component.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SnInteractionScan), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(Owner);
	World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(ScanRadius),
		QueryParams);

	TArray<AActor*> Candidates;
	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* Actor = Result.GetActor();
		// Center-distance gate, same predicate as IsValidTarget: the overlap
		// sphere is only the discovery pass (its reach = ScanRadius + the
		// target's collision extent), so without this filter an interactable
		// with a large collision body would show in the prompt while the F-key
		// execution (center distance <= ScanRadius) rejects it. Prompt list and
		// execution must share the same geometry: visible means interactable.
		if (Actor && GetInteractable(Actor) && !Candidates.Contains(Actor)
			&& FVector::DistSquared(Owner->GetActorLocation(), Actor->GetActorLocation()) <= FMath::Square(ScanRadius))
		{
			Candidates.Add(Actor);
		}
	}

	// Stable ordering: candidates already in the list keep their position (so
	// the wheel selection never jumps); new ones are simply appended.
	TArray<AActor*> NewOnes;
	for (AActor* Actor : Candidates)
	{
		if (!VisibleCandidates.Contains(Actor))
		{
			NewOnes.Add(Actor);
		}
	}

	// Drop candidates that left the scan; keep the rest in order.
	VisibleCandidates.RemoveAll([&Candidates](const AActor* Actor) { return !Candidates.Contains(Actor); });

	VisibleCandidates.Append(NewOnes);
	if (VisibleCandidates.Num() > MaxCandidates)
	{
		VisibleCandidates.SetNum(MaxCandidates);
	}

	BroadcastUpdate();
}

void UPlayerInteractionComponent::CycleSelection(int32 Delta)
{
	if (VisibleCandidates.Num() == 0)
	{
		return;
	}

	// Wraps in both directions.
	SelectedIndex = (SelectedIndex + Delta) % VisibleCandidates.Num();
	if (SelectedIndex < 0)
	{
		SelectedIndex += VisibleCandidates.Num();
	}

	BroadcastUpdate();
}

void UPlayerInteractionComponent::BroadcastUpdate()
{
	// Keep the selection valid across list changes: clamp instead of resetting
	// so the highlighted target stays near where the player was looking.
	if (SelectedIndex >= VisibleCandidates.Num())
	{
		SelectedIndex = VisibleCandidates.Num() > 0 ? VisibleCandidates.Num() - 1 : 0;
	}
	else if (SelectedIndex < 0)
	{
		SelectedIndex = 0;
	}

	// Change detection: the scan runs every 0.15s but the UI must only be
	// rebuilt when the candidates or the selection actually changed.
	bool bChanged = SelectedIndex != LastBroadcastIndex || VisibleCandidates.Num() != LastBroadcastCandidates.Num();
	if (!bChanged)
	{
		for (int32 i = 0; i < VisibleCandidates.Num(); ++i)
		{
			if (VisibleCandidates[i] != LastBroadcastCandidates[i])
			{
				bChanged = true;
				break;
			}
		}
	}

	if (!bChanged)
	{
		return;
	}

	LastBroadcastCandidates = VisibleCandidates;
	LastBroadcastIndex = SelectedIndex;
	OnInteractablesUpdated.Broadcast(VisibleCandidates, SelectedIndex);
}

bool UPlayerInteractionComponent::IsValidTarget(const AActor* Target) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Target || !GetInteractable(Target))
	{
		return false;
	}

	// Distance-only gate (mirrors the scan filter).
	return FVector::DistSquared(Owner->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(ScanRadius);
}

UInteractableComponent* UPlayerInteractionComponent::GetInteractable(const AActor* Target) const
{
	if (!Target)
	{
		return nullptr;
	}
	UInteractableComponent* Comp = Target->FindComponentByClass<UInteractableComponent>();
	// Disabled interactables (pooled pickups) are invisible to the scan and
	// the F-key re-check, so a hidden pickup can never be selected.
	return Comp && Comp->bEnabled ? Comp : nullptr;
}
