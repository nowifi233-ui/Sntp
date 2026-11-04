// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
// FGameplayEffectSpecHandle is a USTRUCT value member (the carried damage
// spec) - full type needed, not a forward declaration.
#include "GameplayEffectTypes.h"
// FGameplayTag member (HitCueTag).
#include "GameplayTagContainer.h"
// FTimerHandle member (the lifetime timeout).
#include "Engine/TimerHandle.h"
#include "ASnProjectile.generated.h"

class UAbilitySystemComponent;
class UPrimitiveComponent;
class USceneComponent;
class USphereComponent;
class UStaticMeshComponent;
class UProjectileMovementComponent;

/**
 * The projectile of the projectile-attack family: spawned by
 * USnGA_ProjectileAttack on the server (bReplicates + replicated movement),
 * flies with the engine's ProjectileMovementComponent, and resolves its hit
 * server-side against the damage spec the launcher pre-built at fire time.
 * Clients see a replicated copy: their own ProjectileMovement replays the
 * same flight (same spawn rotation -> same initial direction at InitialSpeed;
 * the engine derives the initial velocity from the spawn transform, so the
 * launch needs no velocity code), and replicated transforms keep it on the
 * server's path - for homing missiles the client flies straight and the
 * updates drag it along the server's curve (accepted on LAN, upgrade path
 * documented in Docs/GA_PROJECTILE.md).
 *
 * Hit rules (server-only gameplay; the client copy is decorative and never
 * resolves anything):
 * - First enemy contact (same hostile tag source as the whole combat system:
 *   USnCombatLibrary::IsEnemy - allies / companions / neutrals pass through)
 *   applies the pre-built spec, fires the hit cue at the impact point and
 *   destroys the missile (single hit, no pierce - approved scope).
 * - A blocking world hit (wall / ground) stops the flight and destroys it.
 * - LifetimeSeconds after launch the server recycles it (5.7's PMC has no
 *   MaxLifeTime anymore - see the engine cpp), so a miss never flies on.
 *
 * Physics + visuals (speed / gravity / tracking turn rate / mesh / radius)
 * are numbers on the projectile blueprint's CDO; this class only wires the
 * shape and the rules. Server-only data (the carried spec, the source ASC,
 * the attacker) is never replicated.
 */
UCLASS()
class SN_API ASnProjectile : public AActor
{
	GENERATED_BODY()

public:
	ASnProjectile();

	/** Server-side launch data, called right after spawn by the firing GA:
	 *  the pre-built damage spec (applied on hit - the delayed-delivery
	 *  pattern, see USnCombatLibrary::MakeDamageSpec), the attacker (cue
	 *  instigator) and its ASC (spec application + cue execution), and the
	 *  hit cue tag. Never runs on the replicated client copies. */
	void InitializeProjectile(FGameplayEffectSpecHandle InDamageSpec, AActor* InAttacker, UAbilitySystemComponent* InSourceASC, FGameplayTag InHitCueTag);

	/** Starts homing: attaches the runtime homing target (engine rule - the
	 *  component must be set after the projectile is spawned). A target that
	 *  dies mid-flight leaves the missile flying straight (engine-native). */
	void SetHomingTarget(USceneComponent* TargetComponent);

	virtual void BeginPlay() override;

	/** How long a missile that hits nothing lives before the server recycles
	 *  it (also the effective range knob: lifetime x speed). */
	UPROPERTY(EditDefaultsOnly, Category = "Projectile")
	float LifetimeSeconds = 3.f;

protected:
	/** Sweep body: blocks the world (walls/ground stop the flight), overlaps
	 *  pawns for the enemy hit test. */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<USphereComponent> CollisionSphere;

	/** Visual mesh (material picked in the BP). */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** Engine flight (bRotationFollowsVelocity; initial direction comes from
	 *  the spawn rotation at InitialSpeed - see the class doc). */
	UPROPERTY(VisibleAnywhere, Category = "Projectile")
	TObjectPtr<UProjectileMovementComponent> Movement;

	/** Server hit test on enemy contact. UFUNCTION: dynamic delegate handler
	 *  (AddDynamic binds it by name). */
	UFUNCTION()
	void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Server cleanup when the flight is blocked by the world. */
	UFUNCTION()
	void OnProjectileStopped(const FHitResult& ImpactResult);

	/** Server timeout cleanup (LifetimeSeconds). */
	void HandleLifetimeExpired();

private:
	/** The spec pre-built by the launcher; applied to the victim on hit. */
	FGameplayEffectSpecHandle DamageSpecHandle;

	/** The attacker (hit-cue instigator) and its ASC (spec application + cue
	 *  execution on hit). Both weak: the ASC lives on the attacker's
	 *  PlayerState, so a pawn death mid-flight does not break the hit. */
	TWeakObjectPtr<AActor> Attacker;
	TWeakObjectPtr<UAbilitySystemComponent> SourceASC;

	/** Impact cue fired at the hit point (GameplayCue.Hit.<skill>). */
	FGameplayTag HitCueTag;

	/** True once the missile resolved its hit - guards the same-frame overlap
	 *  batch from double hits (the missile dies on its first target). */
	bool bHasHit = false;

	FTimerHandle LifetimeHandle;
};
