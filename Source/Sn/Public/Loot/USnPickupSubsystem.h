// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USnPickupSubsystem.generated.h"

class AActor;
class ASnItemPickup;
class ASnXPPickup;
class UGameplayEffect;

/** One class's inactive instances. Struct wrapper: UHT does not allow a
 *  container (TArray) as a TMap value directly. */
USTRUCT()
struct FSnPickupPool
{
	GENERATED_BODY()

	UPROPERTY(Transient)
	TArray<TObjectPtr<AActor>> Instances;
};

/** Broadcast at the end of every cleanup round (Count = removed from the field
 *  this round - recycled to the pool, or destroyed when the pool was full;
 *  fires every round, zero included - the round cadence itself is the signal). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickupsCleaned, int32, Count);

/** Broadcast when cap eviction destroys a same-type pickup (Count = 1 per
 *  evicted pickup; BP binders aggregate for totals). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickupsEvicted, int32, Count);

/** Broadcast once per cleanup round, PickupCleanupNoticeSeconds before the
 *  sweep runs ("cleanup in N seconds"). Bare signal, no payload - consumers
 *  decide what to show. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCleanupImminent);

/**
 * Global pickup manager + object pool (one per world), the sibling of
 * USnEnemyPoolSubsystem for world pickups (XP shards and item drops).
 *
 * 1. Pool: pickups are spawned and absorbed in bursts (waves of enemies drop
 *    shards four at a time, each absorbed on contact), so instances are
 *    recycled instead of destroyed. Pooled by class; live pickups and pooled
 *    instances are each capped per class at USnSettings::PickupPoolSize
 *    (XP shards and item drops never compete for field or pool space).
 * 2. Field management: every live pickup is registered with its spawn time.
 *    A single cleanup timer (community pattern: one batch sweep instead of a
 *    timer per pickup) recycles pickups past PickupLifetime back to their
 *    class pool (destroyed only when the pool is full), and spawning evicts
 *    the oldest live pickup of the SAME type to make room - so the field
 *    never accumulates, a new drop is never swallowed, and neither type can
 *    starve the other.
 *
 * Dies with the world, so pooled actors are cleaned up on level switch for
 * free (same as the enemy pool).
 */
UCLASS()
class SN_API USnPickupSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	/** Returns a live XP shard: pooled instance if any, otherwise a fresh actor
	 *  while under the XP cap. Evicts the oldest live XP shard to make room
	 *  (item drops have their own cap and are untouched). Configures and
	 *  revives it (callers must not Initialize again). */
	ASnXPPickup* SpawnXPPickup(TSubclassOf<ASnXPPickup> PickupClass, const FVector& Location, const FRotator& Rotation, float XPValue, TSubclassOf<UGameplayEffect> GrantEffectClass);

	/** Same as SpawnXPPickup for item drops (discard drops and enemy loot):
	 *  the item cap evicts only item drops. */
	ASnItemPickup* SpawnItemPickup(TSubclassOf<ASnItemPickup> PickupClass, const FVector& Location, const FRotator& Rotation, FName ItemRowName, int32 Count);

	/** Returns a pickup to its class pool (deactivated). Replaces the class's
	 *  oldest pooled instance when that class's pool is full. Safe on pickups
	 *  that never came from the pool (they are unregistered and destroyed).
	 *  Blueprint-callable so pickup actions can recycle instead of Destroy. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Pickup")
	void ReturnToPool(AActor* Pickup);

	/** Fired at the end of every cleanup round, zero included (see
	 *  FOnPickupsCleaned). Blueprints bind it for observability. */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Pickup")
	FOnPickupsCleaned OnPickupsCleaned;

	/** Fired when cap eviction destroys a pickup (see FOnPickupsEvicted). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Pickup")
	FOnPickupsEvicted OnPickupsEvicted;

	/** Fired before each cleanup round (see FOnCleanupImminent). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Pickup")
	FOnCleanupImminent OnCleanupImminent;

private:
	/** One cleanup pass: drop dead entries, destroy expired pickups. */
	void HandleCleanupTick();

	/** Fires OnCleanupImminent - see FOnCleanupImminent. */
	void HandleCleanupNotice();

	/** Destroys the oldest live pickup of the given type (bXP: XP shards or
	 *  item drops). False when that type has nothing left to evict. */
	bool EvictOldestActive(bool bXP);

	/** Recycles a live pickup into its class pool (or destroys it when full). */
	void RecyclePickup(AActor* Pickup);

	/** Live pickups of the given type (bXP), cast-based like EvictOldestActive
	 *  so BP subclasses count as their base type. */
	int32 CountActive(bool bXP) const;

	/** Live pickups in spawn order (index 0 = oldest). */
	struct FActivePickupEntry
	{
		TWeakObjectPtr<AActor> Pickup;
		/** Last activation time: the entry is recreated per spawn, so pool
		 *  reuse starts a fresh clock. */
		float SpawnTime = 0.f;
	};
	TArray<FActivePickupEntry> ActivePickups;

	/** Inactive instances by class, ready for reuse. */
	UPROPERTY(Transient)
	TMap<TSubclassOf<AActor>, FSnPickupPool> Pools;

	FTimerHandle CleanupTimerHandle;
	FTimerHandle CleanupNoticeTimerHandle;
};
