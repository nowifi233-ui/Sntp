// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "SnCompanionAIController.generated.h"

/**
 * Companion controller: follows the owner like the enemy chases the player
 * (re-pathing only when the previous move finished), and polls for shoot
 * targets on a timer. When a target exists it activates the companion's
 * configured attack skill by class (ASnCompanion::AttackAbilityClass - the
 * same configurable GA slot type as the player's) - the ability itself
 * decides damage and effects, so the cooldown lives in GAS as everywhere
 * else.
 *
 * GetShootTarget() is a BlueprintNativeEvent: the C++ default picks the
 * nearest enemy in range, a BP can override it to decide "what counts as a
 * target" (e.g. only the enemy the player is looking at).
 */
UCLASS()
class SN_API ASnCompanionAIController : public AAIController
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaSeconds) override;

	/** The current shoot target, or null to stay silent. C++ default finds the
	 *  nearest enemy within ShootRadius; override in a BP to change the rule. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "CompanionAI")
	APawn* GetShootTarget() const;
	APawn* GetShootTarget_Implementation() const;

protected:
	virtual void BeginPlay() override;

	/** Polls GetShootTarget() and activates the companion's AttackAbilityClass. */
	void TryShoot();

	/** Distance to the owner beyond which the companion starts following. */
	UPROPERTY(EditDefaultsOnly, Category = "CompanionAI")
	float FollowDistance = 300.f;

	/** How close the companion gets when following. */
	UPROPERTY(EditDefaultsOnly, Category = "CompanionAI")
	float AcceptanceRadius = 100.f;

	/** How far around the companion enemies are considered for shooting. */
	UPROPERTY(EditDefaultsOnly, Category = "CompanionAI")
	float ShootRadius = 800.f;

	/** Seconds between shoot-target polls. */
	UPROPERTY(EditDefaultsOnly, Category = "CompanionAI")
	float ShootInterval = 0.4f;

private:
	FTimerHandle ShootTimerHandle;
};
