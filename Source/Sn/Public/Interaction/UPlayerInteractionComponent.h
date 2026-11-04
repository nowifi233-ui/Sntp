// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UPlayerInteractionComponent.generated.h"

class UInteractableComponent;

/** Broadcast whenever the visible interaction targets or the selection change (timer-driven, not per-frame). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSnInteractablesUpdatedDelegate, TArray<AActor*>, Interactables, int32, SelectedIndex);

/**
 * Perception layer of the interaction system: mounted on the player character.
 * Scans on a timer, keeps the nearby interactables as a single stable-order
 * candidate list (existing entries keep their position, new ones append;
 * mouse wheel cycles the selection), and broadcasts list + selected index.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SN_API UPlayerInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPlayerInteractionComponent();

	/** Broadcast when the visible candidates or the selected index change. */
	UPROPERTY(BlueprintAssignable, Category = "Interaction")
	FSnInteractablesUpdatedDelegate OnInteractablesUpdated;

	/** Latest candidate list (stable order: existing entries never re-sort). */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	const TArray<AActor*>& GetVisibleInteractables() const { return VisibleCandidates; }

	/** Index of the currently selected candidate (mouse wheel cycles it). */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	int32 GetSelectedIndex() const { return SelectedIndex; }

	/** The selected candidate (what interacting would act on). */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	AActor* GetSelectedTarget() const { return VisibleCandidates.IsValidIndex(SelectedIndex) ? VisibleCandidates[SelectedIndex] : nullptr; }

	/** Cycles the selection by Delta (+1 next, -1 previous), wrapping around. */
	void CycleSelection(int32 Delta);

	/** Re-checks that the target is still in range, e.g. before executing an interaction. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsValidTarget(const AActor* Target) const;

	/** Immediately re-scans (called after an interaction completes). */
	void ForceScan();

	/** Radius of the overlap scan. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float ScanRadius = 500.f;

	/** How often the scan runs. Timer-driven, not Tick. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	float ScanInterval = 0.15f;

	/** Cap on how many candidates are broadcast at once. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	int32 MaxCandidates = 8;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartScanning();
	void Scan();

	/** Clamps the selection to the list; broadcasts only when something changed. */
	void BroadcastUpdate();

	UInteractableComponent* GetInteractable(const AActor* Target) const;

	FTimerHandle ScanTimerHandle;
	TArray<AActor*> VisibleCandidates;
	int32 SelectedIndex = 0;

	/** Last broadcast snapshot for change detection (scan runs often, UI refreshes rarely). */
	TArray<AActor*> LastBroadcastCandidates;
	int32 LastBroadcastIndex = -1;
};
