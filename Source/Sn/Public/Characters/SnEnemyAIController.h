// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SnEnemyAIController.generated.h"

/**
 * Simple chase controller: keeps moving the possessed enemy toward the
 * nearest actor tagged Actor_Player (SnGameplayTags) within DetectionRadius,
 * re-pathing only when the previous move finished and re-scanning on an
 * interval while idle. Targets are acquired by tag scan, NOT from the player
 * controller's pawn - the pawn reference can be null right after a level
 * travel (continue game), which used to leave every enemy standing still.
 *
 * The chase stands down while the enemy swings (ASnEnemy::IsAttacking): the
 * attack is the enemy's own business, the controller only holds still for its
 * duration so the move request cannot drag the body through the animation.
 */
UCLASS()
class SN_API ASnEnemyAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	/** How close the enemy must get before it stops approaching. Small on
	 *  purpose: the chase closes right up to the player (the leap and the
	 *  attack, both in SnEnemy::Tick, handle the "engage from range" part -
	 *  a swing fires from AttackRange, long before the chase reaches this). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float AcceptanceRadius = 10.f;

	/** Radius around the enemy in which tagged actors wake it up and get
	 *  chased (the nearest one wins). Enemies idle while nobody is inside. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float DetectionRadius = 800.f;

	/** Beyond this distance the chased actor is dropped and the enemy idles
	 *  in place again. Larger than DetectionRadius (hysteresis) so a target
	 *  hovering on the detection border doesn't flicker the chase on/off. */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float LoseRadius = 1200.f;

	/** Seconds between target scans while idle (no per-frame actor scans). */
	UPROPERTY(EditDefaultsOnly, Category = "AI")
	float RescanInterval = 0.5f;

	/** The actor currently being chased (null while idle). ASnEnemy uses this
	 *  for the leap and to trigger its attack, so "who we chase is who we
	 *  fight" stays a single source of truth. */
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

private:
	/** Nearest Actor_Player-tagged actor within DetectionRadius, or null. */
	AActor* FindNearestTarget() const;

	/** Weak: drops automatically when the target is destroyed. */
	TWeakObjectPtr<AActor> CurrentTarget;

	float LastScanTime = 0.f;
};
