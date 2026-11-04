// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SnSurvivalSpawner.generated.h"

class ASnEnemy;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnSpawnerStateChangedDelegate, bool, bRunning);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnWaveChangedDelegate, int32, Wave);

/**
 * Survival-style enemy flood spawner (Vampire Survivors-like), started and
 * stopped through an interactable switch - or, with bAutoStart, the moment the
 * level loads. Spawns enemies on a ring around itself, scales their stats with
 * the wave count and the menu's difficulty multiplier, and takes instances from
 * the global USnEnemyPoolSubsystem. Gameplay only - instance management lives
 * in the pool.
 */
UCLASS()
class SN_API ASnSurvivalSpawner : public AActor
{
	GENERATED_BODY()

public:
	ASnSurvivalSpawner();

	/** Starts the flood (wave resets to 1). No-op while already running. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void Start();

	/** Stops spawning and returns all owned enemies to the pool (field cleared). */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void Stop();

	/** Starts or stops depending on the current state. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void Toggle();

	UFUNCTION(BlueprintPure, Category = "Spawner")
	bool IsRunning() const { return bRunning; }

	/** Current wave number (1-based). */
	UFUNCTION(BlueprintPure, Category = "Spawner")
	int32 GetDifficultyWave() const { return DifficultyWave; }

	/** Global difficulty multiplier (the menu sets it before Start). */
	UFUNCTION(BlueprintPure, Category = "Spawner")
	float GetDifficultyMultiplier() const { return DifficultyMultiplier; }

	/** All configured enemy IDs (the menu pre-checks these). */
	UFUNCTION(BlueprintPure, Category = "Spawner")
	const TArray<FName>& GetEnemyIDs() const { return EnemyIDs; }

	/** Enemies currently out of the pool (menu status line). */
	UFUNCTION(BlueprintPure, Category = "Spawner")
	int32 GetAliveCount() const;

	/** Difficulty scale applied on top of the wave multipliers (0.6 / 1.0 / 1.5...). */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SetDifficultyMultiplier(float InMultiplier);

	/** Enemy IDs the menu enabled; spawns pick randomly from this list. */
	UFUNCTION(BlueprintCallable, Category = "Spawner")
	void SetEnabledEnemyIDs(const TArray<FName>& InIDs);

	/** Broadcast when Start/Stop flips the running state. */
	UPROPERTY(BlueprintAssignable, Category = "Spawner")
	FSnSpawnerStateChangedDelegate OnSpawnerStateChanged;

	/** Broadcast when the wave advances. */
	UPROPERTY(BlueprintAssignable, Category = "Spawner")
	FSnWaveChangedDelegate OnWaveChanged;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Spawns one enemy, then reschedules (the interval shrinks with the wave). */
	void HandleSpawnTimer();

	/** Advances the difficulty wave. */
	void HandleWaveTimer();

	/** Ring candidate around this actor, verified against the NavMesh (up to 8 attempts). */
	bool FindSpawnLocation(FVector& OutLocation) const;

	/** Vertical probe at the candidate's X/Y that snaps it onto the ground
	 *  below (ground + capsule half height, so the capsule bottom rests on
	 *  the surface). Returns false when no ground is found - the caller
	 *  skips the candidate instead of spawning in the void. */
	bool SnapToGround(FVector& Point) const;

	/** The player died: stop the flood and clear the field (death/restart flow comes later). */
	UFUNCTION()
	void HandlePlayerDied(AActor* Killer);

	/** One of our enemies finished its death sequence: drop it from tracking. */
	UFUNCTION()
	void HandleEnemyDied(AActor* Enemy);

	/** Spawn interval for the current wave (difficulty divides it). */
	float GetCurrentSpawnInterval() const;

	/** Start the flood on BeginPlay instead of waiting for the interaction
	 *  switch. EditAnywhere (not EditDefaultsOnly like the tuning knobs below):
	 *  "opens by itself" is a per-instance decision - one level can hold an
	 *  auto-starting flood next to a switch-driven one. Still the menu's
	 *  Start/Stop as usual once running. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner", meta = (DisplayName = "放入地图自动开启"))
	bool bAutoStart = false;

	/** Enemy class the pool instantiates (e.g. BP_Enemy). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "敌人类型"))
	TSubclassOf<ASnEnemy> EnemyClass;

	/** Row names to pick from when spawning (the menu can enable a subset). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "敌人 ID 列表"))
	TArray<FName> EnemyIDs;

	/** Distance from this actor where enemies appear (off-screen ring). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "生成环半径"))
	float SpawnRingRadius = 1200.f;

	/** Spawn interval at wave 1 (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "初始生成间隔（秒）"))
	float InitialSpawnInterval = 1.2f;

	/** Hard floor for the spawn interval. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "最小生成间隔（秒）"))
	float MinSpawnInterval = 0.3f;

	/** Interval removed per wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "每波间隔衰减（秒）"))
	float SpawnIntervalDecayPerWave = 0.1f;

	/** Seconds per wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "每波时长（秒）"))
	float WaveDuration = 30.f;

	/** Extra health fraction per wave (wave 2 = 1.15x, and so on). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "每波血量增幅"))
	float HealthPerWave = 0.15f;

	/** Extra attack fraction per wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "每波攻击增幅"))
	float AttackPerWave = 0.10f;

	/** Extra move speed fraction per wave. */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "每波速度增幅"))
	float SpeedPerWave = 0.02f;

	/** Speed multiplier ceiling (capped before the difficulty scale applies). */
	UPROPERTY(EditDefaultsOnly, Category = "Spawner", meta = (DisplayName = "速度倍率上限"))
	float MaxSpeedMultiplier = 1.5f;

private:
	/** Spawns one enemy from the pool and configures it for the current wave. */
	void SpawnEnemyOnce();

	/** The difficulty scale currently driving spawns: the player's tier
	 *  multiplier (PlayerState - server-authoritative, the menu writes it via
	 *  the SetDifficulty RPC) when a player state exists, else the legacy
	 *  float state (SetDifficultyMultiplier - kept for old callers). */
	float GetEffectiveDifficultyMultiplier() const;

	bool bRunning = false;
	int32 DifficultyWave = 1;
	float DifficultyMultiplier = 1.f;

	/** Enemy IDs the menu enabled; empty = nothing spawns. */
	TArray<FName> EnabledEnemyIDs;

	/** Enemies we spawned and have not returned yet (Stop() clears the field). */
	UPROPERTY(Transient)
	TArray<TObjectPtr<ASnEnemy>> OwnedEnemies;

	FTimerHandle SpawnTimerHandle;
	FTimerHandle WaveTimerHandle;
};
