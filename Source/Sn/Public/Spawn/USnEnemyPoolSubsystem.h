// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "USnEnemyPoolSubsystem.generated.h"

class ASnEnemy;

/** One class's inactive instances. UHT cannot reflect a TArray as a TMap
 *  value (no container nesting), so the bucket is a struct: keyed lookup by
 *  EnemyClass stays O(buckets) - pools are small, linear is fine. */
USTRUCT()
struct FSnEnemyPoolBucket
{
	GENERATED_BODY()

	/** Concrete enemy class this bucket holds. */
	UPROPERTY()
	TSubclassOf<ASnEnemy> EnemyClass;

	/** Inactive, deactivated instances of EnemyClass, ready for reuse. */
	UPROPERTY()
	TArray<TObjectPtr<ASnEnemy>> Instances;
};

/**
 * Global enemy object pool (one per world). Owns enemy *instances*: dead
 * enemies are deactivated and recycled instead of destroyed, so spawning
 * never pays actor-creation costs mid-game. Gameplay logic (when, where, how
 * strong) lives in the spawner - this subsystem only hands out instances and
 * tracks the active count. Dies with the world, so pooled actors are cleaned
 * up on level switch for free.
 */
UCLASS()
class SN_API USnEnemyPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Returns a reusable enemy instance: from the pool if any, otherwise a fresh
	 *  actor while under the global cap (USnSettings::EnemyPoolSize). Null at capacity. */
	ASnEnemy* SpawnEnemy(TSubclassOf<ASnEnemy> EnemyClass, const FVector& Location, const FRotator& Rotation);

	/** Returns the enemy to the pool (deactivated). Destroys it when the pool is
	 *  full. Safe to call on an enemy that is already in the pool. */
	void ReturnToPool(ASnEnemy* Enemy);

	/** Enemies currently out of the pool (alive or dying). */
	int32 GetActiveCount() const { return ActiveEnemies.Num(); }

	/** Inactive enemies waiting in the pool (all buckets). */
	int32 GetPooledCount() const
	{
		int32 Total = 0;
		for (const FSnEnemyPoolBucket& Bucket : Pool)
		{
			Total += Bucket.Instances.Num();
		}
		return Total;
	}

private:
	/** Inactive, deactivated enemies ready to be reused, bucketed by concrete
	 *  class (BP_Enemy, BP_Enemy_Tutorial, ...). A bucket only ever holds its
	 *  own class, so recycling can never resurrect a subclass's blueprint
	 *  behaviour (an instance-level OnEnemyDeath binding would survive
	 *  ConfigureForSpawn) as a different class. */
	UPROPERTY(Transient)
	TArray<FSnEnemyPoolBucket> Pool;

	/** Enemies currently out of the pool (spawned, not yet returned). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ASnEnemy>> ActiveEnemies;
};
