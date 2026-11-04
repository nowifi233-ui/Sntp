// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/SnSettings.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "USnDifficultyLibrary.generated.h"

class UAbilitySystemComponent;

/**
 * Difficulty tier logic: table lookup (USnSettings::DifficultyDataTable) and
 * the player-side difficulty effect. The effect is a transient Infinite GE
 * (MultiplyCompound modifiers on the player's vital attributes, granted tag
 * Difficulty.Applied), built in code instead of one asset per tier so the
 * table can grow without new effect assets (same pattern as the level-up
 * effect in ASnPlayerState::CheckForLevelUp).
 *
 * Server-authoritative flow: the menu picks a tier via
 * SnPlayerController::SetDifficulty (Server RPC) -> GameState::Difficulty
 * (session-scoped, replicated); on entering a combat zone the GameMode
 * applies it here, and the spawner reads GameState::GetDifficultyMultiplier()
 * for enemy scaling (RoR2: the tier belongs to the run, not the player).
 */
UCLASS()
class SN_API USnDifficultyLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** C++ lookup: row for a tier ID, or nullptr if unknown. */
	static const FSnDifficultyRow* FindDifficultyRow(FName DifficultyRowName);

	/** Every tier row in table order, RowName filled from the table key (the
	 *  spawner menu lists these and dispatches the picked RowName). */
	UFUNCTION(BlueprintPure, Category = "Sn|Difficulty")
	static TArray<FSnDifficultyRow> GetAllDifficultyRows();

	/** Applies the tier's player multipliers as one Infinite GE on the player
	 *  ASC, removing any previous difficulty effect first (re-entering the
	 *  combat zone must never stack). Returns false when the ASC is missing. */
	static bool ApplyDifficultyToPlayer(UAbilitySystemComponent* ASC, const FSnDifficultyRow& Row);

	/** Removes the active difficulty effect (the hub strips it on arrival so
	 *  the player returns to base values). */
	static void RemoveDifficultyFromPlayer(UAbilitySystemComponent* ASC);
};
