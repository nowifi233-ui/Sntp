// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawn/USnEnemyPoolSubsystem.h"
#include "Characters/SnEnemy.h"
#include "Game/SnSettings.h"

ASnEnemy* USnEnemyPoolSubsystem::SpawnEnemy(TSubclassOf<ASnEnemy> EnemyClass, const FVector& Location, const FRotator& Rotation)
{
	if (!EnemyClass)
	{
		return nullptr;
	}

	ASnEnemy* Enemy = nullptr;
	// Pull from the requested class's own bucket: a recycled instance is
	// always exactly the class the caller asked for, so a subclass carrying
	// blueprint death logic (a tutorial enemy) can never come back as a plain
	// enemy. Drop the bucket when it empties - the list only holds buckets
	// that can actually give an instance.
	for (int32 i = Pool.Num() - 1; i >= 0; --i)
	{
		if (Pool[i].EnemyClass != EnemyClass)
		{
			continue;
		}
		if (Pool[i].Instances.Num() > 0)
		{
			Enemy = Pool[i].Instances.Pop();
			if (Pool[i].Instances.IsEmpty())
			{
				Pool.RemoveAt(i);
			}
		}
		break;
	}
	if (!Enemy && ActiveEnemies.Num() + GetPooledCount() < USnSettings::Get()->GetEnemyPoolSize())
	{
		// No pooled instance of this class and the global cap allows one more:
		// spawn a fresh one. The cap bounds active + pooled (across all class
		// buckets), so total actor count never grows past
		// USnSettings::EnemyPoolSize - the game stops spawning, never overflows.
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Enemy = GetWorld()->SpawnActor<ASnEnemy>(EnemyClass, Location, Rotation, Params);
	}
	if (!Enemy)
	{
		return nullptr; // At capacity; the caller skips this tick.
	}

	// Reposition for both paths (reused actors come back from their last spot).
	Enemy->SetActorLocation(Location, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	Enemy->SetActorRotation(Rotation);
	ActiveEnemies.Add(Enemy);
	return Enemy;
}

void USnEnemyPoolSubsystem::ReturnToPool(ASnEnemy* Enemy)
{
	if (!Enemy)
	{
		return;
	}

	ActiveEnemies.Remove(Enemy);

	// Already pooled - e.g. the spawner cleared the field while a death
	// sequence was still falling, and the sequence finished afterwards.
	for (const FSnEnemyPoolBucket& Bucket : Pool)
	{
		if (Bucket.EnemyClass == Enemy->GetClass() && Bucket.Instances.Contains(Enemy))
		{
			return;
		}
	}

	if (GetPooledCount() + ActiveEnemies.Num() < USnSettings::Get()->GetEnemyPoolSize())
	{
		Enemy->DeactivateForPool();

		// Back into this instance's own class bucket (create the bucket lazily
		// the first time the class is recycled).
		bool bBucketed = false;
		for (FSnEnemyPoolBucket& Bucket : Pool)
		{
			if (Bucket.EnemyClass == Enemy->GetClass())
			{
				Bucket.Instances.Add(Enemy);
				bBucketed = true;
				break;
			}
		}
		if (!bBucketed)
		{
			FSnEnemyPoolBucket NewBucket;
			NewBucket.EnemyClass = Enemy->GetClass();
			NewBucket.Instances.Add(Enemy);
			Pool.Add(NewBucket);
		}
	}
	else
	{
		Enemy->Destroy();
	}
}
