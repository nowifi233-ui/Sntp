// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASnItemPickup.generated.h"

class UInteractableComponent;
class USnDropMotionComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * An item lying in the world: spawned by drops, by discarding from the
 * inventory, or placed by designers. Launched by the shared drop-motion
 * component (ballistic arc -> bounce -> float), same as XP shards. Carries
 * only the item ID + count and lets the interaction system pick it up
 * (F key, same as chests).
 */
UCLASS()
class SN_API ASnItemPickup : public AActor
{
	GENERATED_BODY()

public:
	ASnItemPickup();

	/** Configures the stack this pickup represents (item ID + count). */
	void Initialize(FName InItemRowName, int32 InCount);

	/** Fired at the end of Initialize: the item data (row name + count) is set.
	 *  Subclasses use this for per-item visuals (e.g. the paper-card texture).
	 *  Hand-placed pickups never initialize, so the event never fires for them. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Item")
	void OnPickupInitialized();

	/** Revives a pooled pickup: visible, interactable again, drop motion
	 *  restarted (called by USnPickupSubsystem on both fresh spawns and pool
	 *  reuse). */
	void ActivateForPool();

	/** Puts the pickup back into its pool: hidden, interaction disabled
	 *  (called by USnPickupSubsystem on recycle). */
	void DeactivateForPool();

	virtual void BeginPlay() override;

	UFUNCTION(BlueprintPure, Category = "Item")
	FName GetItemRowName() const { return ItemRowName; }

	UFUNCTION(BlueprintPure, Category = "Item")
	int32 GetItemCount() const { return Count; }

	UFUNCTION(BlueprintPure, Category = "Item")
	UInteractableComponent* GetInteractableComponent() const { return InteractableComponent; }

	//~ AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End of AActor interface

	/** Clients: the replicated item data arrived - apply the per-item visual
	 *  (the server already calls OnPickupInitialized from Initialize). */
	UFUNCTION()
	void OnRep_PickupData();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<USphereComponent> CollisionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Drives the interaction prompt + pickup action (row "ItemPickup" by default). */
	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<UInteractableComponent> InteractableComponent;

	/** Shared ballistic drop animation (arc -> bounce -> float). */
	UPROPERTY(VisibleAnywhere, Category = "Item")
	TObjectPtr<USnDropMotionComponent> DropMotion;

	/** Item data table row name (the item's ID). Replicated: clients apply the
	 *  per-item visual when it arrives (OnRep_PickupData). */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_PickupData, Category = "Item")
	FName ItemRowName = NAME_None;

	/** Stack size this pickup represents. */
	UPROPERTY(EditAnywhere, Replicated, Category = "Item")
	int32 Count = 1;
};
