// Fill out your copyright notice in the Description page of Project Settings.


#include "Spawn/SnSurvivalSpawner.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "Engine/World.h"
#include "Characters/SnEnemy.h"
#include "Game/SnGameState.h"
#include "Spawn/USnEnemyPoolSubsystem.h"
#include "NavigationSystem.h"
#include "Components/CapsuleComponent.h"
#include "Player/SnPlayerState.h"

ASnSurvivalSpawner::ASnSurvivalSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void ASnSurvivalSpawner::BeginPlay()
{
	Super::BeginPlay();

	// The menu pre-checks every configured enemy ID.
	EnabledEnemyIDs = EnemyIDs;

	// The flood stops when the player dies (death/restart flow comes later).
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ASnPlayerState* SnPS = PC->GetPlayerState<ASnPlayerState>())
		{
			if (USnAttributeSet* AttributeSet = SnPS->GetAttributeSet())
			{
				AttributeSet->OnDeath.AddDynamic(this, &ASnSurvivalSpawner::HandlePlayerDied);
			}
		}
	}

	// Placed-in-map auto start. Server only: Start() spawns actors and arms
	// timers, so a client-side copy would only litter the client with
	// unreplicated enemies the pool can never reclaim - and the flood is
	// server-authoritative like every other gameplay decision. HasAuthority()
	// is true in Standalone, so single player is unaffected.
	//
	// Runs after EnabledEnemyIDs is seeded above (Start() spawns immediately).
	// Known edge: on "continue game" the saved difficulty is restored later
	// (RestartPlayer -> HandlePlayerEnteredZone), so the very first enemy is
	// scaled by the default tier and every one after it by the restored one.
	if (bAutoStart && HasAuthority())
	{
		Start();
	}
}

void ASnSurvivalSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (ASnPlayerState* SnPS = PC->GetPlayerState<ASnPlayerState>())
		{
			if (USnAttributeSet* AttributeSet = SnPS->GetAttributeSet())
			{
				AttributeSet->OnDeath.RemoveDynamic(this, &ASnSurvivalSpawner::HandlePlayerDied);
			}
		}
	}
	for (ASnEnemy* Enemy : OwnedEnemies)
	{
		if (Enemy)
		{
			Enemy->OnEnemyDied.RemoveDynamic(this, &ASnSurvivalSpawner::HandleEnemyDied);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void ASnSurvivalSpawner::Start()
{
	if (bRunning)
	{
		return;
	}

	bRunning = true;
	DifficultyWave = 1;
	GetWorldTimerManager().SetTimer(WaveTimerHandle, this, &ASnSurvivalSpawner::HandleWaveTimer, WaveDuration, /*bLoop=*/true);
	HandleSpawnTimer(); // Spawns immediately, then reschedules with the current interval.
	OnWaveChanged.Broadcast(DifficultyWave); // Wave banner shows "第 1 波"; later waves broadcast in HandleWaveTimer.
	OnSpawnerStateChanged.Broadcast(true);
}

void ASnSurvivalSpawner::Stop()
{
	if (!bRunning)
	{
		return;
	}

	bRunning = false;
	GetWorldTimerManager().ClearTimer(SpawnTimerHandle);
	GetWorldTimerManager().ClearTimer(WaveTimerHandle);

	// Return everything we spawned to the pool. Death sequences are cut short
	// here (DeactivateForPool hides and stops them), so no late OnEnemyDied
	// can fire afterwards and the pool sees no double-return.
	if (USnEnemyPoolSubsystem* Pool = GetWorld()->GetSubsystem<USnEnemyPoolSubsystem>())
	{
		for (ASnEnemy* Enemy : OwnedEnemies)
		{
			if (Enemy)
			{
				Enemy->OnEnemyDied.RemoveDynamic(this, &ASnSurvivalSpawner::HandleEnemyDied);
				Pool->ReturnToPool(Enemy);
			}
		}
	}
	OwnedEnemies.Empty();
	OnSpawnerStateChanged.Broadcast(false);
}

void ASnSurvivalSpawner::Toggle()
{
	if (bRunning)
	{
		Stop();
	}
	else
	{
		Start();
	}
}

void ASnSurvivalSpawner::HandleSpawnTimer()
{
	if (!bRunning)
	{
		return;
	}

	SpawnEnemyOnce();
	GetWorldTimerManager().SetTimer(SpawnTimerHandle, this, &ASnSurvivalSpawner::HandleSpawnTimer, GetCurrentSpawnInterval(), /*bLoop=*/false);
}

void ASnSurvivalSpawner::SpawnEnemyOnce()
{
	if (EnabledEnemyIDs.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnSurvivalSpawner %s: no enabled enemy IDs (fill EnemyIDs or the menu unchecked everything)"), *GetName());
		return;
	}
	if (!EnemyClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnSurvivalSpawner %s: EnemyClass is not set"), *GetName());
		return;
	}

	USnEnemyPoolSubsystem* Pool = GetWorld()->GetSubsystem<USnEnemyPoolSubsystem>();
	if (!Pool)
	{
		return;
	}

	FVector SpawnLocation;
	FindSpawnLocation(SpawnLocation); // Never fails: falls back to a raw ring point.

	ASnEnemy* Enemy = Pool->SpawnEnemy(EnemyClass, SpawnLocation, GetActorRotation());
	if (!Enemy)
	{
		// Pool at capacity.
		UE_LOG(LogTemp, Warning, TEXT("ASnSurvivalSpawner %s: pool at capacity, spawn skipped"), *GetName());
		return;
	}

	// Wave multipliers x the difficulty scale (the player's tier - see
	// GetEffectiveDifficultyMultiplier).
	const int32 Wave = DifficultyWave;
	const float DifficultyScale = GetEffectiveDifficultyMultiplier();
	const float HealthMult = (1.f + HealthPerWave * (Wave - 1)) * DifficultyScale;
	const float AttackMult = (1.f + AttackPerWave * (Wave - 1)) * DifficultyScale;
	const float SpeedMult = FMath::Min(1.f + SpeedPerWave * (Wave - 1), MaxSpeedMultiplier) * DifficultyScale;

	Enemy->ConfigureForSpawn(EnabledEnemyIDs[FMath::RandHelper(EnabledEnemyIDs.Num())], HealthMult, AttackMult, SpeedMult);

	// Track it so Stop() can clear the field. Pool reuse re-issues the same
	// instance, so unbind before binding to keep exactly one listener.
	Enemy->OnEnemyDied.RemoveDynamic(this, &ASnSurvivalSpawner::HandleEnemyDied);
	Enemy->OnEnemyDied.AddDynamic(this, &ASnSurvivalSpawner::HandleEnemyDied);
	OwnedEnemies.Add(Enemy);
}

bool ASnSurvivalSpawner::FindSpawnLocation(FVector& OutLocation) const
{
	UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(GetWorld());

	// Last ground-projected candidate: used when a NavData exists but rejects
	// every ring point (the usual auto-created empty RecastNavMesh).
	FVector Fallback = FVector::ZeroVector;
	bool bHasFallback = false;

	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		FVector Candidate = GetActorLocation() + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SpawnRingRadius;

		// Per-point ground projection: each candidate stands on the ground
		// under its own X/Y (never the spawner's height - that buries enemies
		// on uneven terrain). No ground below (cliff edge etc.): skip the
		// candidate, never spawn in the void.
		if (!SnapToGround(Candidate))
		{
			continue;
		}

		if (!bHasFallback)
		{
			Fallback = Candidate;
			bHasFallback = true;
		}

		// No usable NavMesh (no NavMeshBoundsVolume / not built): accept the
		// ground-projected point so spawning never stalls - the enemies just
		// can't chase. GetCurrent() is almost never null (the engine always
		// has a NavigationSystem), so the NavData check is the real fallback.
		if (!NavSys || !NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate))
		{
			OutLocation = Candidate;
			return true;
		}

		FNavLocation NavLocation;
		if (NavSys->GetRandomReachablePointInRadius(Candidate, 100.f, NavLocation))
		{
			// The NavMesh point's Z is the walking surface, not the spawn
			// height - re-project its X/Y the same way as every candidate.
			OutLocation = NavLocation;
			SnapToGround(OutLocation); // No ground here is vanishingly rare; NavMesh wins.
			return true;
		}
	}

	// All attempts rejected: a NavData exists but has no usable data for the
	// ring (auto-created empty RecastNavMesh with no volume, or not built).
	// Fall back to the last ground-projected ring point so the flood still
	// spawns; the enemies just can't chase.
	if (bHasFallback)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnSurvivalSpawner %s: NavMesh rejected the ring 8/8, falling back to a ground-projected ring point (place a NavMeshBoundsVolume and Build Paths)"), *GetName());
		OutLocation = Fallback;
		return true;
	}
	return false;
}

bool ASnSurvivalSpawner::SnapToGround(FVector& Point) const
{
	if (!GetWorld())
	{
		return false;
	}

	// Vertical probe at this candidate's X/Y: from well above down to far
	// below. A miss (e.g. the ring hangs over a cliff edge) means no usable
	// ground - the caller skips the candidate instead of spawning in the void.
	const FVector TraceStart = Point + FVector(0.f, 0.f, 2000.f);
	const FVector TraceEnd = Point - FVector(0.f, 0.f, 2000.f);
	FHitResult Hit;
	if (!GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic))
	{
		return false;
	}

	// The spawn position is the capsule center: standing on the ground means
	// ground + capsule half height, with a small margin so the capsule bottom
	// rests just above the surface. Half height comes from the enemy class
	// CDO, so capsule tweaks in the BP stay aligned automatically.
	float HalfHeight = 88.f;
	if (EnemyClass)
	{
		if (ASnEnemy* EnemyCDO = EnemyClass->GetDefaultObject<ASnEnemy>())
		{
			HalfHeight = EnemyCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
	}
	Point.Z = Hit.ImpactPoint.Z + HalfHeight + 5.f;
	return true;
}

void ASnSurvivalSpawner::HandleWaveTimer()
{
	++DifficultyWave;
	OnWaveChanged.Broadcast(DifficultyWave);
}

float ASnSurvivalSpawner::GetCurrentSpawnInterval() const
{
	const float BaseInterval = FMath::Max(MinSpawnInterval, InitialSpawnInterval - SpawnIntervalDecayPerWave * (DifficultyWave - 1));
	return FMath::Max(MinSpawnInterval, BaseInterval / FMath::Max(GetEffectiveDifficultyMultiplier(), KINDA_SMALL_NUMBER));
}

float ASnSurvivalSpawner::GetEffectiveDifficultyMultiplier() const
{
	// Session-scoped tier (RoR2): the run's difficulty lives on the GameState
	// (set by the lobby / spawner menu through the controller RPC), so every
	// player sees the same enemy scaling. The local float is the fallback for
	// levels without a GameState (legacy setups).
	if (UWorld* World = GetWorld())
	{
		if (ASnGameState* GameState = World->GetGameState<ASnGameState>())
		{
			return GameState->GetDifficultyMultiplier();
		}
	}
	return DifficultyMultiplier;
}

void ASnSurvivalSpawner::HandlePlayerDied(AActor* Killer)
{
	Stop();
}

void ASnSurvivalSpawner::HandleEnemyDied(AActor* Enemy)
{
	OwnedEnemies.Remove(Cast<ASnEnemy>(Enemy));
}

void ASnSurvivalSpawner::SetDifficultyMultiplier(float InMultiplier)
{
	DifficultyMultiplier = FMath::Max(InMultiplier, 0.1f);
}

void ASnSurvivalSpawner::SetEnabledEnemyIDs(const TArray<FName>& InIDs)
{
	EnabledEnemyIDs = InIDs;
}

int32 ASnSurvivalSpawner::GetAliveCount() const
{
	if (USnEnemyPoolSubsystem* Pool = GetWorld()->GetSubsystem<USnEnemyPoolSubsystem>())
	{
		return Pool->GetActiveCount();
	}
	return 0;
}
