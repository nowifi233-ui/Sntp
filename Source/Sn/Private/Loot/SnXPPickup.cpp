// Fill out your copyright notice in the Description page of Project Settings.


#include "Loot/SnXPPickup.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameplayEffect.h"
#include "Kismet/GameplayStatics.h"
#include "Math/NumericLimits.h"
#include "Loot/USnDropMotionComponent.h"
#include "Loot/USnPickupSubsystem.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"

ASnXPPickup::ASnXPPickup()
{
	PrimaryActorTick.bCanEverTick = true;
	// Replicated so clients see the shard fly in (all gameplay - magnetism,
	// absorb, XP grant - stays server-side; the client copy is visual only).
	SetReplicates(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetSphereRadius(18.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionProfileName(TEXT("OverlapAll"));
	RootComponent = CollisionSphere;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	DropMotion = CreateDefaultSubobject<USnDropMotionComponent>(TEXT("DropMotion"));
}

void ASnXPPickup::Initialize(float InXPValue, TSubclassOf<UGameplayEffect> InGrantEffectClass)
{
	XPValue = InXPValue;
	GrantEffectClass = InGrantEffectClass;
}

void ASnXPPickup::ActivateForPool()
{
	// Revive: visible, ticking again, drop motion restarted (LaunchDrop is
	// re-entrant - it re-rolls the velocity and the settle timer).
	SetActorHiddenInGame(false);
	SetActorTickEnabled(true);
	if (DropMotion)
	{
		DropMotion->SetComponentTickEnabled(true);
		DropMotion->LaunchDrop();
	}
	// Magnetism shrinks the shard on the way in; pooled shards always come
	// back full size.
	SetActorScale3D(FVector::OneVector);
}

void ASnXPPickup::DeactivateForPool()
{
	// Inert in the pool: Tick is the only per-frame cost, so stop it (plus
	// no rendering, no collisions).
	SetActorTickEnabled(false);
	if (DropMotion)
	{
		DropMotion->SetComponentTickEnabled(false);
	}
	SetActorHiddenInGame(true);
}

void ASnXPPickup::BeginPlay()
{
	Super::BeginPlay();
	DropMotion->LaunchDrop();
}

void ASnXPPickup::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AActor* Player = GetNearestPlayerPawn();

	// Absorb on contact at any point of the lifetime (even mid-arc). Server
	// only: the client copy flies in for the visual but never absorbs - the
	// server's recycle destroys it.
	if (GetLocalRole() == ROLE_Authority
		&& Player && FVector::Dist(GetActorLocation(), Player->GetActorLocation()) < AbsorbRadius)
	{
		Absorb(Player);
		return;
	}

	// Magnetism only once the shard has settled into its float animation.
	// AttractTowards pauses the drop motion so the float doesn't fight the flight.
	if (Player && DropMotion->IsSettled()
		&& FVector::Dist(GetActorLocation(), Player->GetActorLocation()) < MagnetRadius)
	{
		AttractTowards(Player, DeltaTime);
	}
}

void ASnXPPickup::AttractTowards(AActor* Player, float DeltaTime)
{
	// The magnetized flight owns the location now; stop the float animation
	// until the shard is absorbed (and the actor, with the component, destroyed).
	DropMotion->SetComponentTickEnabled(false);

	// Fly toward the player with an accelerating curve (closer = faster, with a
	// little final rush), and shrink for a "consumed" feel.
	const FVector Target = Player->GetActorLocation();
	const float Dist = FVector::Dist(GetActorLocation(), Target);
	const float Speed = MagnetSpeed * FMath::Lerp(2.5f, 1.f, FMath::Clamp(Dist / MagnetRadius, 0.f, 1.f));
	const FVector NewLocation = FMath::VInterpTo(GetActorLocation(), Target, DeltaTime, Speed);
	SetActorLocation(NewLocation);
	const float Scale = FMath::Max(0.3f, GetActorScale3D().X - 0.5f * DeltaTime);
	SetActorScale3D(FVector(Scale));

	if (FVector::Dist(NewLocation, Target) < AbsorbRadius)
	{
		// Client copy: stop here and wait for the server's absorb to recycle
		// the real shard (the copy disappears with it). The server's magnet
		// target can differ from the local player, so its position is never
		// replicated onto the copy - that would fight the local simulation.
		if (GetLocalRole() == ROLE_Authority)
		{
			Absorb(Player);
		}
	}
}

void ASnXPPickup::Absorb(AActor* Player)
{
	// Server-only gameplay: XP granting from a client would never reach the
	// server's authoritative attributes, and recycling from a client would
	// desync the pool (guard for safety - the call sites already gate).
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// RoR2 team XP: one player absorbs, everyone gains - each member settles
	// their share with their own multiplier (a placeholder 1.0 for now, planned
	// as an attribute). Same grant path the enemy used before the loot redesign:
	// a SetByCaller XP effect, so level-up logic (PlayerState) is untouched.
	if (AGameStateBase* GameState = GetWorld()->GetGameState())
	{
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			ASnPlayerState* SnPS = Cast<ASnPlayerState>(PlayerState);
			UAbilitySystemComponent* ASC = SnPS ? SnPS->GetAbilitySystemComponent() : nullptr;
			if (!ASC || !GrantEffectClass)
			{
				continue;
			}

			const float Amount = XPValue * SnPS->GetXPGetMultiplier();
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(this);
			const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(GrantEffectClass, 1.f, Context);
			if (Spec.IsValid())
			{
				Spec.Data->SetSetByCallerMagnitude(SnGameplayTags::SetByCaller_XP, Amount);
				ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}

		}

	}

	// Recycle instead of destroy: absorbed shards are the highest-frequency
	// spawn/destroy churn in the game (a wave of enemies absorbs in seconds).
	// Falls back to Destroy when the subsystem is unavailable (e.g. shutdown).
	if (USnPickupSubsystem* PickupSystem = GetWorld()->GetSubsystem<USnPickupSubsystem>())
	{
		PickupSystem->ReturnToPool(this);
	}
	else
	{
		Destroy();
	}
}

AActor* ASnXPPickup::GetNearestPlayerPawn() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (World->GetNetMode() == NM_Client)
	{
		// Client-side visual simulation: magnet toward the LOCAL player. The
		// server still resolves absorption with its own nearest-player query;
		// the client copy just flies to its own viewer (and disappears when
		// the server recycles the real pickup).
		return UGameplayStatics::GetPlayerPawn(this, 0);
	}

	// Server picks the magnet target: the nearest living player (standalone has
	// exactly one). Multiplayer must not always magnet toward the first
	// controller - on a listen server that is the host, and every shard would
	// fly home instead of to whoever is actually closest.
	if (AGameStateBase* GameState = World->GetGameState())
	{
		AActor* Nearest = nullptr;
		float NearestDistSq = TNumericLimits<float>::Max();
		const FVector ShardLocation = GetActorLocation();
		for (APlayerState* PlayerState : GameState->PlayerArray)
		{
			APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
			if (!Pawn)
			{
				continue;
			}
			const float DistSq = FVector::DistSquared(Pawn->GetActorLocation(), ShardLocation);
			if (DistSq < NearestDistSq)
			{
				NearestDistSq = DistSq;
				Nearest = Pawn;
			}
		}
		return Nearest;
	}
	return nullptr;
}
