// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// Full types required by UHT: TArray<FSnDropEntry> (a USTRUCT) and
// TSubclassOf<ASnItemPickup> (pickable class pins need the definition).
#include "Inventory/FSnItemTypes.h"
#include "Loot/ASnItemPickup.h"
#include "ASnDestructible.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class USoundBase;

/**
 * Breakable world prop (crate, urn, ...). Takes hits by COUNT, not damage: it
 * carries no ASC, so the attack cores (sphere family + projectile) route it
 * through a special branch (USnCombatLibrary::FindDestructible) that counts
 * one hit per landed attack regardless of the attacker's damage numbers.
 * Each hit and the break itself play their feedback (sound, and a Niagara burst
 * on break) on every side through multicasts, while the drops go through the
 * pickup subsystem (server only). Scene-placed, single-run: breaking is not
 * persisted (leaving and re-entering the level resets it).
 */
UCLASS()
class SN_API ASnDestructible : public AActor
{
	GENERATED_BODY()

public:
	ASnDestructible();

	/** One attack landed (server calls only - filtered inside). Counts down;
	 *  at zero the destructible breaks (VFX + drops) and hides itself. */
	void RegisterHit();

protected:
	virtual void BeginPlay() override;

private:
	/** Hit feedback on every side. Sent for the hits that leave it standing -
	 *  the last one is the break and plays the break feedback instead. */
	UFUNCTION(NetMulticast, Reliable)
	void MulticastHit();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastBreak();

	/** Drops every configured entry above the mesh. Server only. */
	void SpawnDrops();

	/** Attacks needed to break this. */
	UPROPERTY(EditDefaultsOnly, Category = "Destructible")
	int32 HitPointsToBreak = 3;

	/** Item drops, same row format as enemy drops (FSnDropEntry). */
	UPROPERTY(EditDefaultsOnly, Category = "Destructible")
	TArray<FSnDropEntry> Drops;

	/** Pickup actor class for the drops (the project default item pickup). */
	UPROPERTY(EditDefaultsOnly, Category = "Destructible")
	TSubclassOf<ASnItemPickup> ItemPickupClass;

	/** Niagara burst played once on every side at break time (null = vanish
	 *  with no VFX). */
	UPROPERTY(EditDefaultsOnly, Category = "Destructible")
	TObjectPtr<UNiagaraSystem> BreakVFX;

	/** Break sound, once on every side (null = silent). Attenuation and mix come
	 *  from the asset - nothing is overridden here. */
	UPROPERTY(EditDefaultsOnly, Category = "Destructible")
	TObjectPtr<USoundBase> BreakSound;

	/** Sound of a hit that did NOT break it, once on every side (null = silent).
	 *  Same asset-driven attenuation as BreakSound. */
	UPROPERTY(EditDefaultsOnly, Category = "Destructible")
	TObjectPtr<USoundBase> HitSound;

	UPROPERTY(VisibleAnywhere, Category = "Destructible")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Hits left before breaking. Server-authoritative (never replicated). */
	int32 HitsRemaining = 0;

	/** True once broken and hidden (guard against double breaks). */
	bool bBroken = false;
};
