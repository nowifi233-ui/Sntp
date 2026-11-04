// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USnBuildPreviewComponent.generated.h"

class ASnBuildPreview;
class USnGA_Build;
class USnInventoryComponent;

/**
 * Shows where the held build item would put a device, tinted by whether that
 * spot would be accepted. Local presentation of a local decision: driven by
 * THIS machine's selection and cooldown, spawning a ghost only this machine can
 * see, and never touching the placement itself - USnGA_Build stays the only
 * thing that spawns a device, on the server.
 *
 * "The player is holding a build item" is read off the ABILITY SYSTEM, not off
 * the item table: the hotbar grants the selected slot's UseAbility while that
 * item is the one in hand (USnInventoryComponent::RefreshHotbarGrant), and a
 * granted spec replicates to its owner, so this machine simply asks its own ASC
 * whether a USnGA_Build is held. The device numbers then come from that granted
 * spec - the same object the server activates when the item is used.
 *
 * The tick runs at all times (its first lines are early-outs) instead of being
 * switched on by the selection: the selection and the grant are two independent
 * replication streams, and the base UAbilitySystemComponent has no
 * ability-granted callback to wake up on (5.8: activated / ended / committed /
 * failed only). Event-driven, a client could receive the selection first, find
 * no spec yet, switch the tick off, and never wake up again - the ghost would
 * simply never appear.
 *
 * The ghost says exactly what a use would do right now: no item left in the
 * bag, cooldown still running, a window open (UI-only input - the use input
 * cannot fire), or no ground under the aim all hide it rather than showing a
 * ghost that promises something the server would refuse.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SN_API USnBuildPreviewComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USnBuildPreviewComponent();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	/** The build ability this machine is holding, or null. Two replicated
	 *  streams have to agree - the selected hotbar row and the granted spec -
	 *  before a ghost is drawn at all: they arrive independently on a client,
	 *  and the honest answer for the frame or two they disagree is "nothing in
	 *  hand", not a ghost for an item the player has stopped holding. */
	const USnGA_Build* FindSelectedBuildAbility() const;

	/** The owning pawn's inventory. It lives on the PLAYER STATE, not the pawn
	 *  (the project's GAS ownership rule), and reading the selection through
	 *  the wrong object is a silent "no item selected" - hence one accessor
	 *  both callers share. Null until the player state exists. */
	const USnInventoryComponent* GetInventoryComponent() const;

	/** Hands the ghost back without destroying it: the next frame may want it
	 *  again, and a hidden actor costs nothing. SetActorHiddenInGame early-outs
	 *  internally, so calling it per frame is free. */
	void HideGhost();

	/** The ghost actor, reused across device switches (ASnBuildPreview::
	 *  SetPreviewMesh swaps its mesh); this machine's own - never replicated. */
	UPROPERTY(Transient)
	TObjectPtr<ASnBuildPreview> GhostActor;

	/** Device the live ghost is currently showing (diff key - the mesh is only
	 *  swapped when the held ability moves to another device). */
	FName GhostDeviceID;
};
