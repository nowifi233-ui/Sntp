// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SnXPPickup.generated.h"

class UGameplayEffect;
class USnDropMotionComponent;
class USphereComponent;
class UStaticMeshComponent;

/**
 * XP shard dropped by dying enemies. Launched by the shared drop-motion
 * component (ballistic arc -> bounce -> float), then magnetized toward the
 * nearest player once settled; absorbed on contact. Absorption is team-wide
 * (RoR2): the server grants the shard's XP to every player - each settles
 * their own share with their own multiplier (GetXPGetMultiplier) - and the
 * "+XP" popup goes to the absorber's client only. Server-authoritative:
 * the grant, the absorb and the recycle all run on the server.
 *
 * Motion is client-side simulated: the drop-motion component replicates the
 * server's rolled initial velocity and every client replays the same arc
 * locally; once settled, each client magnetizes the shard toward ITS OWN
 * player (the server magnetizes toward the nearest player). The client copy
 * never absorbs - it stops at the player and disappears when the server
 * recycles the real shard.
 */
UCLASS()
class SN_API ASnXPPickup : public AActor
{
	GENERATED_BODY()

public:
	ASnXPPickup();

	/** Called by the dying enemy: how much XP this shard grants and which effect grants it. */
	void Initialize(float InXPValue, TSubclassOf<UGameplayEffect> InGrantEffectClass);

	/** Revives a pooled shard: visible, ticking, drop motion restarted (called
	 *  by USnPickupSubsystem on both fresh spawns and pool reuse). */
	void ActivateForPool();

	/** Puts the shard back into its pool: hidden, not ticking (called by
	 *  USnPickupSubsystem on recycle). */
	void DeactivateForPool();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

protected:
	/** Physical body: bounces on spawn, disabled once the shard settles. */
	UPROPERTY(VisibleAnywhere, Category = "XP Pickup")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Visual mesh (e.g. a glowing gem; material picked in the BP). */
	UPROPERTY(VisibleAnywhere, Category = "XP Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Shared ballistic drop animation (arc -> bounce -> float). */
	UPROPERTY(VisibleAnywhere, Category = "XP Pickup")
	TObjectPtr<USnDropMotionComponent> DropMotion;

	/** Player inside this radius starts attracting the shard (0 disables magnetism). */
	UPROPERTY(EditDefaultsOnly, Category = "XP Pickup")
	float MagnetRadius = 300.f;

	/** Distance at which the shard is absorbed on contact. */
	UPROPERTY(EditDefaultsOnly, Category = "XP Pickup")
	float AbsorbRadius = 100.f;

	/** How fast the shard accelerates toward the player when magnetized. */
	UPROPERTY(EditDefaultsOnly, Category = "XP Pickup")
	float MagnetSpeed = 12.f;

private:
	/** Magnetized tick: fly toward the player, shrink, absorb on contact. */
	void AttractTowards(AActor* Player, float DeltaTime);

	/** Grants the XP effect to the whole team and recycles the shard. */
	void Absorb(AActor* Player);

	/** The nearest player pawn (the magnet target): the server picks the
	 *  nearest across all players (never just the host), a client picks its
	 *  own local player for the visual simulation. */
	AActor* GetNearestPlayerPawn() const;

	float XPValue = 0.f;
	TSubclassOf<UGameplayEffect> GrantEffectClass;
};
