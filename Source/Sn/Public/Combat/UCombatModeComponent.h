// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UCombatModeComponent.generated.h"

class ASnEnemy;

/** Broadcast when the local player enters or leaves combat (combat = an enemy is within range). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnCombatModeChangedDelegate, bool, bInCombat);

/**
 * Combat state of the LOCAL player, computed per client (the enemy health
 * bars and the camera both follow this). Scans for nearby enemies on a timer,
 * turns on the overhead health bar of enemies entering the radius and turns
 * it off when they leave - driven directly, never via global state, so in
 * multiplayer each client only sees bars for its own combat.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class SN_API UCombatModeComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UCombatModeComponent();

	/** Broadcast when the combat state flips. */
	UPROPERTY(BlueprintAssignable, Category = "CombatMode")
	FSnCombatModeChangedDelegate OnCombatModeChanged;

	/** True while at least one enemy is within CombatRadius. */
	UFUNCTION(BlueprintPure, Category = "CombatMode")
	bool IsInCombat() const { return bInCombat; }

	/** Radius within which enemies count as combat. */
	UPROPERTY(EditDefaultsOnly, Category = "CombatMode")
	float CombatRadius = 800.f;

	/** How often the scan runs. Timer-driven, not Tick. */
	UPROPERTY(EditDefaultsOnly, Category = "CombatMode")
	float ScanInterval = 0.2f;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void StartScanning();
	void Scan();

	FTimerHandle ScanTimerHandle;

	/** Enemies currently showing their health bar (diffed each scan). */
	TArray<ASnEnemy*> EnemiesInCombat;

	bool bInCombat = false;
};
