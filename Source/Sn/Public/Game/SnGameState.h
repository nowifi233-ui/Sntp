// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "SnGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnDifficultyChangedDelegate, FName, NewDifficulty);

/**
 * Session-wide state, replicated to every client: the run's difficulty tier.
 * RoR2-style semantics: the tier belongs to the run/session, not to any
 * single player - picked from the spawner menu, shared by everyone in the
 * zone. It survives world switches (and save/continue cycles) through the
 * snapshot: the player state restores it on world entry (TryRestoreFromSave),
 * so a fresh GameState's "Normal" default only applies to brand-new runs.
 */
UCLASS()
class SN_API ASnGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	/** The run's difficulty tier ID (row name in USnSettings
	 *  DifficultyDataTable). Replicated: every client converges on the run's
	 *  tier. "Normal" is the default until the menu picks a tier. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	FName GetDifficulty() const { return Difficulty; }

	/** Server-only: sets the run's difficulty tier. The menu's SetDifficulty
	 *  RPC routes here; the value replicates to clients (OnRep broadcast). */
	void SetDifficulty(FName InDifficulty);

	/** The tier's enemy multiplier (the spawner scales enemies by this).
	 *  Falls back to 1.0 when the tier or the difficulty table is missing. */
	UFUNCTION(BlueprintPure, Category = "Difficulty")
	float GetDifficultyMultiplier() const;

	/** Broadcast when the run's difficulty changes (server after
	 *  SetDifficulty, clients on replication). The spawner menu listens to
	 *  refresh its selection on other machines. */
	UPROPERTY(BlueprintAssignable, Category = "Difficulty")
	FSnDifficultyChangedDelegate OnDifficultyChanged;

	//~ AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End of AActor interface

protected:
	/** Replicated change notification: forwards the new tier to local widgets. */
	UFUNCTION()
	void OnRep_Difficulty();

private:
	UPROPERTY(ReplicatedUsing = OnRep_Difficulty)
	FName Difficulty = FName(TEXT("Normal"));
};
