// Fill out your copyright notice in the Description page of Project Settings.


#include "Loot/USnPickupSubsystem.h"

#include "Loot/SnXPPickup.h"
#include "Engine/World.h"
#include "Loot/ASnItemPickup.h"
#include "Game/SnSettings.h"
#include "TimerManager.h"

void USnPickupSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);

	const USnSettings* Settings = USnSettings::Get();
	const float Interval = Settings ? Settings->GetPickupCleanupInterval() : 0.f;
	const float Notice = Settings ? Settings->GetPickupCleanupNoticeSeconds() : 0.f;
	if (Interval > 0.f)
	{
		// One batch timer for all pickups: with heavy drops, a timer per
		// pickup would pile up callbacks (the community pattern for loot
		// cleanup is a single scanning interval).
		GetWorld()->GetTimerManager().SetTimer(CleanupTimerHandle, this, &USnPickupSubsystem::HandleCleanupTick, Interval, /*bLoop=*/true);

		// Parallel heads-up timer: first fire Interval - Notice after
		// BeginPlay, then every Interval - so it always precedes the cleanup
		// round by Notice seconds. The plain SetTimer(Rate, bLoop) overload
		// loops at the first delay too, which would drift out of phase with
		// the round; hence the InFirstDelay overload.
		if (Notice > 0.f && Notice < Interval)
		{
			GetWorld()->GetTimerManager().SetTimer(CleanupNoticeTimerHandle, this, &USnPickupSubsystem::HandleCleanupNotice, Interval, /*bLoop=*/true, /*InFirstDelay=*/Interval - Notice);
		}
		else if (Notice > 0.f)
		{
			// Misconfigured lead time: with Notice >= Interval there is no
			// "before the round" moment to fire at - say so instead of a
			// silently missing notification.
			UE_LOG(LogTemp, Warning, TEXT("USnPickupSubsystem: PickupCleanupNoticeSeconds (%.1f) >= PickupCleanupInterval (%.1f), pre-round notice disabled"), Notice, Interval);
		}
	}
}

void USnPickupSubsystem::HandleCleanupNotice()
{
	// Bare heads-up before each round - see FOnCleanupImminent.
	UE_LOG(LogTemp, Log, TEXT("USnPickupSubsystem: cleanup round imminent"));
	OnCleanupImminent.Broadcast();
}

void USnPickupSubsystem::Deinitialize()
{
	// The cleanup timer holds a raw `this`. In practice it dies with the world
	// (a world subsystem's lifetime is the world's lifetime), but clear it
	// explicitly while the world - and its timer manager - still exist.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(CleanupTimerHandle);
		World->GetTimerManager().ClearTimer(CleanupNoticeTimerHandle);
	}
	Super::Deinitialize();
}

ASnXPPickup* USnPickupSubsystem::SpawnXPPickup(TSubclassOf<ASnXPPickup> PickupClass, const FVector& Location, const FRotator& Rotation, float XPValue, TSubclassOf<UGameplayEffect> GrantEffectClass)
{
	// Server-only: pickups spawn on the authority and replicate to clients -
	// the client copy of this subsystem is an empty shell (its pickups are
	// replicated, never local), and spawning here would only create a ghost.
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	if (!PickupClass || !GetWorld())
	{
		return nullptr;
	}

	// Make room within this class's own live cap. XP shards and item drops
	// are capped independently (each up to PickupPoolSize live), so neither
	// type ever evicts the other - eviction only sheds the same type.
	while (CountActive(/*bXP=*/true) >= USnSettings::Get()->GetPickupPoolSize())
	{
		if (!EvictOldestActive(/*bXP=*/true))
		{
			return nullptr;
		}
	}

	ASnXPPickup* Pickup = nullptr;
	if (FSnPickupPool* ClassPool = Pools.Find(PickupClass); ClassPool && ClassPool->Instances.Num() > 0)
	{
		AActor* Raw = ClassPool->Instances.Pop();
		Pickup = Cast<ASnXPPickup>(Raw);
		if (!Pickup)
		{
			// Class-keyed pools guarantee the type; just drop the odd one out.
			Raw->Destroy();
		}
	}
	else
	{
		// Pool empty for this class: spawn fresh. The pool cap is enforced at
		// recycle time (RecyclePickup replaces the class's oldest), so there
		// is nothing to shed here.
		FActorSpawnParameters Params;
		// Same as the item path: shards pop out around the corpse, which can sit
		// inside geometry (enemies die against walls) - nudge out of it instead
		// of spawning inside.
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Pickup = GetWorld()->SpawnActor<ASnXPPickup>(PickupClass, Location, Rotation, Params);
	}

	if (!Pickup)
	{
		return nullptr;
	}

	// Reposition for both paths (reused shards come back from their last spot),
	// then re-initialize the data and revive (fresh instances already ran
	// LaunchDrop in BeginPlay; pooled ones need it restarted here).
	Pickup->SetActorLocation(Location, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	Pickup->SetActorRotation(Rotation);
	Pickup->Initialize(XPValue, GrantEffectClass);
	Pickup->ActivateForPool();

	ActivePickups.Add({ Pickup, static_cast<float>(GetWorld()->GetTimeSeconds()) });
	return Pickup;
}

ASnItemPickup* USnPickupSubsystem::SpawnItemPickup(TSubclassOf<ASnItemPickup> PickupClass, const FVector& Location, const FRotator& Rotation, FName ItemRowName, int32 Count)
{
	// Server-only (see SpawnXPPickup).
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		return nullptr;
	}

	if (!PickupClass || !GetWorld())
	{
		return nullptr;
	}

	// Item drops get their own live cap (see SpawnXPPickup): evict the oldest
	// item drop only.
	while (CountActive(/*bXP=*/false) >= USnSettings::Get()->GetPickupPoolSize())
	{
		if (!EvictOldestActive(/*bXP=*/false))
		{
			return nullptr;
		}
	}

	ASnItemPickup* Pickup = nullptr;
	if (FSnPickupPool* ClassPool = Pools.Find(PickupClass); ClassPool && ClassPool->Instances.Num() > 0)
	{
		AActor* Raw = ClassPool->Instances.Pop();
		Pickup = Cast<ASnItemPickup>(Raw);
		if (!Pickup)
		{
			Raw->Destroy();
		}
	}
	else
	{
		FActorSpawnParameters Params;
		// Discard drops spawn right in front of the player; nudge out of
		// geometry instead of spawning inside it.
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Pickup = GetWorld()->SpawnActor<ASnItemPickup>(PickupClass, Location, Rotation, Params);
	}

	if (!Pickup)
	{
		return nullptr;
	}

	Pickup->SetActorLocation(Location, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	Pickup->SetActorRotation(Rotation);
	Pickup->Initialize(ItemRowName, Count);
	Pickup->ActivateForPool();

	ActivePickups.Add({ Pickup, static_cast<float>(GetWorld()->GetTimeSeconds()) });
	return Pickup;
}

void USnPickupSubsystem::ReturnToPool(AActor* Pickup)
{
	// Server-only: recycling must hit the server's pools - a client-side
	// recycle only deactivates the caller's own copy (the server's pickup
	// stays alive and replicates back). All legal callers (absorption,
	// server-executed pickup actions, cleanup) are on the authority.
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	// IsValid covers null and destroyed actors (BP can hold stale references
	// to pickups that were already destroyed outside the subsystem).
	if (!IsValid(Pickup) || !GetWorld())
	{
		return;
	}

	// Unregister from the live field, whatever happens below (also drops any
	// dead entries the sweep has not touched yet).
	ActivePickups.RemoveAll([Pickup](const FActivePickupEntry& Entry)
	{
		return !Entry.Pickup.IsValid() || Entry.Pickup.Get() == Pickup;
	});

	// Already pooled (e.g. the field was cleared while a pickup was being
	// absorbed): nothing to do.
	if (FSnPickupPool* ClassPool = Pools.Find(Pickup->GetClass()); ClassPool && ClassPool->Instances.Contains(Pickup))
	{
		return;
	}

	RecyclePickup(Pickup);
}

void USnPickupSubsystem::RecyclePickup(AActor* Pickup)
{
	// Defensive: see ReturnToPool - never park a destroyed actor.
	if (!IsValid(Pickup))
	{
		return;
	}

	// Deactivate (per-class logic: visibility, ticking, interaction) and park
	// it in its class pool; the incoming instance replaces the class's oldest
	// pooled one when the pool is at its cap.
	if (ASnXPPickup* XP = Cast<ASnXPPickup>(Pickup))
	{
		XP->DeactivateForPool();
	}
	else if (ASnItemPickup* Item = Cast<ASnItemPickup>(Pickup))
	{
		Item->DeactivateForPool();
	}
	else
	{
		Pickup->Destroy();
		return;
	}

	// Deactivation is inert today; if a subclass ever self-destructs during
	// it, do not park a garbage-marked actor.
	if (!IsValid(Pickup))
	{
		return;
	}

	FSnPickupPool* ClassPool = Pools.Find(Pickup->GetClass());
	if (ClassPool && ClassPool->Instances.Num() >= USnSettings::Get()->GetPickupPoolSize())
	{
		// This class's pool at its own cap: the incoming instance replaces the
		// class's oldest pooled one. Each class pool is capped independently,
		// so XP shards and item drops never compete for pool space.
		AActor* Oldest = ClassPool->Instances[0];
		ClassPool->Instances.RemoveAt(0);
		Oldest->Destroy();
	}

	if (!ClassPool)
	{
		ClassPool = &Pools.FindOrAdd(Pickup->GetClass());
	}
	ClassPool->Instances.Add(Pickup);
}

void USnPickupSubsystem::HandleCleanupTick()
{
	// Server-only: the client's live list is always empty (its pickups are
	// replicated) - a cleanup round there scans nothing and would broadcast
	// OnPickupsCleaned on the client.
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	// Defensive: the timer is cleared on deinitialize, but never assume.
	if (!GetWorld())
	{
		return;
	}

	const USnSettings* Settings = USnSettings::Get();
	const float Lifetime = Settings ? Settings->GetPickupLifetime() : 60.f;
	const float Now = static_cast<float>(GetWorld()->GetTimeSeconds());

	// One cleanup round: scan every live pickup's age since its last
	// activation (the entry is recreated per spawn, so pool reuse starts a
	// fresh clock), recycle the expired ones back to the pool. Expiry is
	// quantized to the sweep interval - a pickup lives between Lifetime and
	// Lifetime + Interval depending on its drop phase relative to the round
	// (accepted trade-off: a coarse round beats a per-second scan of the
	// whole field).
	int32 Cleaned = 0;
	for (int32 i = ActivePickups.Num() - 1; i >= 0; --i)
	{
		const FActivePickupEntry& Entry = ActivePickups[i];
		AActor* Pickup = Entry.Pickup.Get();
		if (!Pickup)
		{
			// Destroyed outside the subsystem (e.g. scene despawn): drop the
			// dead entry so the count stays accurate.
			ActivePickups.RemoveAtSwap(i);
			continue;
		}
		if (Now - Entry.SpawnTime >= Lifetime)
		{
			// Expired: off the field and back into the class pool for reuse
			// (deactivated). When that class's pool is full, RecyclePickup
			// destroys the class's oldest pooled instance instead - at most one
			// destroy per recycle, and the pool never grows past its cap.
			ActivePickups.RemoveAtSwap(i);
			RecyclePickup(Pickup);
			++Cleaned;
		}
	}

	// Round-end announcement: the count for this cleanup round. Every round
	// is announced, zero included - the round cadence itself is the signal
	// the consumer wants to display.
	UE_LOG(LogTemp, Log, TEXT("USnPickupSubsystem: cleanup round expired %d pickups"), Cleaned);
	OnPickupsCleaned.Broadcast(Cleaned);
}

bool USnPickupSubsystem::EvictOldestActive(bool bXP)
{
	// Index 0 is the oldest (entries are appended in spawn order). The live
	// cap is per type, so only the requested type is shed - XP shards and
	// item drops never evict each other.
	for (int32 i = 0; i < ActivePickups.Num(); ++i)
	{
		AActor* Pickup = ActivePickups[i].Pickup.Get();
		if (!Pickup || (bXP != (Cast<ASnXPPickup>(Pickup) != nullptr)))
		{
			continue;
		}
		ActivePickups.RemoveAt(i);
		Pickup->Destroy();
		OnPickupsEvicted.Broadcast(1);
		return true;
	}

	// Unreachable in theory (CountActive uses the same cast test on the same
	// entries, so count >= cap guarantees a match), but the spawn callers
	// fail closed on this return - log loudly instead of swallowing a drop
	// silently.
	UE_LOG(LogTemp, Warning, TEXT("USnPickupSubsystem::EvictOldestActive: %d live %s at cap %d, nothing evictable"), CountActive(bXP), bXP ? TEXT("XP shards") : TEXT("item drops"), USnSettings::Get()->GetPickupPoolSize());
	return false;
}

int32 USnPickupSubsystem::CountActive(bool bXP) const
{
	int32 Count = 0;
	for (const FActivePickupEntry& Entry : ActivePickups)
	{
		if (Entry.Pickup.IsValid() && (bXP == (Cast<ASnXPPickup>(Entry.Pickup.Get()) != nullptr)))
		{
			++Count;
		}
	}
	return Count;
}
