// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/ASnDestructible.h"

#include "Components/StaticMeshComponent.h"
#include "Inventory/FSnItemTypes.h"
#include "Inventory/USnItemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Loot/ASnItemPickup.h"
#include "Loot/USnPickupSubsystem.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"

ASnDestructible::ASnDestructible()
{
	PrimaryActorTick.bCanEverTick = false;

	// Replicated so the broken (hidden) state reaches every client. The actor
	// stays alive after breaking on purpose: destroying it would close its
	// channel and could drop the queued break RPC.
	SetReplicates(true);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	// Pawn-typed collision so the attack cores see it: the sphere family
	// overlaps by object type (ECC_Pawn) and the projectile sphere responds to
	// the Pawn channel - both skip actors without Pawn-typed collision.
	// Blocks pawns (a solid obstacle; the BP mesh's simple collision shapes
	// it) and ignores everything else: blocking WorldDynamic would eat
	// projectiles instead of letting them reach the overlap hit test (the
	// projectile owns that side of the matrix - its sphere overlaps Pawn).
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_Pawn);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	Mesh->SetGenerateOverlapEvents(true);
}

void ASnDestructible::BeginPlay()
{
	Super::BeginPlay();

	// Snapshot the BP-configured hit count (EditDefaultsOnly values are baked
	// into the CDO after the constructor ran, so this must happen at BeginPlay).
	HitsRemaining = HitPointsToBreak;
}

void ASnDestructible::RegisterHit()
{
	// Server-authoritative: the sphere core also runs its predicted client
	// copy, which must not consume hits - the server's own run accounts for
	// them (the client copy plays the cue locally and absorbs the echo).
	if (!HasAuthority() || bBroken || HitsRemaining <= 0)
	{
		return;
	}

	if (--HitsRemaining > 0)
	{
		// Still standing: hit feedback only. The hit that finishes it skips this
		// and plays the break feedback instead (a break reads as a break, not as
		// a hit stacked on one).
		MulticastHit();
		return;
	}

	bBroken = true;

	// Replicated hide + collision off: the broken state reaches every client
	// through actor replication (collision off also means no further attack
	// can overlap it - no double breaks, no double drops).
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);

	// The burst and the break sound play on every side (a multicast runs locally
	// on the server too), then the drops spawn server-side.
	MulticastBreak();
	SpawnDrops();
}

void ASnDestructible::MulticastHit_Implementation()
{
	if (HitSound)
	{
		// Each machine plays its own copy out of the multicast; no attenuation
		// override, so the asset's own settings decide the falloff.
		UGameplayStatics::PlaySoundAtLocation(this, HitSound, GetActorLocation());
	}
}

void ASnDestructible::MulticastBreak_Implementation()
{
	if (BreakVFX && GetWorld())
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), BreakVFX,
			GetActorLocation(), GetActorRotation(), FVector::OneVector,
			/*bAutoDestroy=*/true, /*bAutoActivate=*/true, ENCPoolMethod::None,
			/*bPreCullCheck=*/true);
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, GetActorLocation());
	}
}

void ASnDestructible::SpawnDrops()
{
	if (!ItemPickupClass)
	{
		return;
	}
	USnPickupSubsystem* PickupSystem = GetWorld()->GetSubsystem<USnPickupSubsystem>();
	if (!PickupSystem)
	{
		return;
	}

	for (const FSnDropEntry& Entry : Drops)
	{
		if (!USnItemLibrary::IsValidItem(Entry.ItemRowName) || Entry.MaxCount < 1)
		{
			continue;
		}
		if (FMath::FRand() > Entry.Chance)
		{
			continue;
		}
		const int32 DropCount = FMath::RandRange(FMath::Max(1, Entry.MinCount), Entry.MaxCount);

		// Toss from above the mesh; the drop motion scatters them outward.
		const FVector SpawnLocation = GetActorLocation() + FVector(
			FMath::FRandRange(-40.f, 40.f),
			FMath::FRandRange(-40.f, 40.f),
			90.f);
		PickupSystem->SpawnItemPickup(ItemPickupClass, SpawnLocation, GetActorRotation(), Entry.ItemRowName, DropCount);
	}
}
