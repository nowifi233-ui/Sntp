// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameplayCueInterface.h"
#include "Components/WidgetComponent.h"
#include "Engine/DataTable.h"
#include "GameplayAbilitySpecHandle.h"
#include "Combat/USnCombatLibrary.h"
#include "Inventory/FSnItemTypes.h"
#include "GameFramework/Character.h"
#include "SnEnemy.generated.h"

/**
 * One row in the global enemy data table (USnSettings). The row name is the
 * enemy's ID, e.g. "Goblin".
 */
USTRUCT(BlueprintType)
struct FSnEnemyInfo : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MaxHealth = 100.f;

	/** Base physical damage. Seeds the PhysicalDamage attribute (the column
	 *  keeps the old name so existing table values stay valid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float AttackPower = 10.f;

	/** XP granted to the killer when this enemy dies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 XPReward = 50;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	float MoveSpeed = 600.f;

	/** Shown over the enemy's head (the only info visible outside combat). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	int32 Level = 1;

	/** Item drops rolled on death (each entry rolls Chance, then Min..Max count). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy")
	TArray<FSnDropEntry> Drops;
};

class UAbilitySystemComponent;
class UGameplayAbility;
class UGameplayEffect;
class UMaterialInterface;
class UUserWidget;
class USnAttributeSet;
class USnEnemyStatusWidget;
class ASnItemPickup;
class ASnXPPickup;
struct FOnAttributeChangeData;
class FLifetimeProperty;

/**
 * Broadcast when the death fall finishes, right before the enemy is returned
 * to the pool (or destroyed). Trackers drop their reference here.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnEnemyDiedDelegate, AActor*, Enemy);

/**
 * Broadcast once when THIS enemy actually dies (Health hits zero), with the
 * killer (may be null) - the moment the death sequence starts, ~2s earlier
 * than OnEnemyDied (which announces the finished fall + pool return). Fires
 * server-only, inside the attribute set's OnDeath call chain. Blueprint
 * special logic (one-shot deaths, opening doors, quest hooks) binds here -
 * do NOT Destroy the enemy from it: recycling is owned by the pool.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnEnemyDeathDelegate, AActor*, Killer);

/**
 * Enemy character. Unlike the player (ASC on PlayerState), the ASC and
 * attribute set live here as subobjects - the standard GAS pattern for AI.
 * Applies default attributes on begin play and flashes red on damage taken.
 *
 * The character itself deals no damage: its attack is a gameplay ability like
 * the player's (USnGA_EnemyAttack, granted at init and activated from Tick
 * while the chase target stands inside AttackRange).
 */
UCLASS()
class SN_API ASnEnemy : public ACharacter, public IAbilitySystemInterface, public IGameplayCueInterface
{
	GENERATED_BODY()

public:
	ASnEnemy();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End of IAbilitySystemInterface

	UFUNCTION(BlueprintPure, Category = "AbilitySystem")
	USnAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** XP granted to the killer. Filled from the data table at init. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetXPReward() const { return XPReward; }

	/** Level shown over the head. Filled from the data table at init. */
	UFUNCTION(BlueprintPure, Category = "Enemy")
	int32 GetEnemyLevel() const { return Level; }

	/** Shows/hides the overhead health bar. Called by the local player's
	 *  combat mode component - the enemy never decides this itself. */
	void SetHealthBarVisible(bool bVisible);

	/** True while a swing is in progress (the attack ability instance is
	 *  running = the animation is playing). The AI controller reads this to
	 *  hold still until the swing is over, and Tick reads it so the movement
	 *  stop happens once per swing instead of every frame. */
	bool IsAttacking() const;

	/** Broadcast once the death fall finishes, just before the enemy returns
	 *  to the pool (or is destroyed). */
	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FSnEnemyDiedDelegate OnEnemyDied;

	/** Broadcast once when this enemy actually dies (death sequence starts). */
	UPROPERTY(BlueprintAssignable, Category = "Enemy")
	FSnEnemyDeathDelegate OnEnemyDeath;

	/** Prepares a (pooled) instance for spawning: resets all state, applies the
	 *  data table row for the ID, then the difficulty multipliers. */
	void ConfigureForSpawn(FName InEnemyID, float HealthMultiplier, float AttackMultiplier, float SpeedMultiplier);

	/** Hides and stops a dead enemy for pool reuse (the pool destroys instead
	 *  when full). */
	void DeactivateForPool();

	//~ AActor interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End of AActor interface

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

	/** Initializes the ASC (owner = avatar = self) and applies default attributes. */
	void InitAbilityActorInfo();

	/** Binds the health delegate that drives the hit flash. */
	void BindAttributeCallbacks();

	/** Looks up this enemy's row in the global enemy data table. */
	const FSnEnemyInfo* FindEnemyInfo() const;

	/** Applies the row's stats to the ASC and movement. */
	void ApplyEnemyInfo(const FSnEnemyInfo& Info);

	/** The enemy died (from the attribute set's death broadcast): drop XP shards, then play the death sequence. */
	UFUNCTION()
	void HandleDeath(AActor* Killer);

	/** Spawns the XP shards (XPReward split into XPShardCount pieces, last one takes the remainder). */
	void SpawnXPPickups();

	/** Rolls the table's drop list and spawns item pickups (same drop delay as the XP shards). */
	void SpawnItemPickups();

	/** Both drops, after the drop delay (the death fall has started by then). */
	void SpawnDropLoot();

	/** Death presentation: keep the hit-flash material, disable movement/collision, stop the AI, start the fall. */
	void StartDeathSequence();

	/** The replayable part of the death sequence: captures the lying pose and
	 *  starts the fall timer. The server runs it directly (and multicasts the
	 *  EnemyDeathFall cue); clients replay it when the cue arrives - the mesh
	 *  fall is a local-only transform and never replicates. */
	void StartDeathFall();

	/** Per-frame fall: rotates the mesh from upright to lying down, then recycles the enemy. */
	void HandleDeathFallTick();

	/** Scales MaxHealth/PhysicalDamage/move speed for the wave and heals to full. */
	void ApplyDifficultyMultiplier(float HealthMultiplier, float AttackMultiplier, float SpeedMultiplier);

	/** Ends the running attack ability, if any: the swing is cut short and its
	 *  montage comes down with it. Called when the enemy dies and when the pool
	 *  recycles it - a corpse must never land a pending hit frame (see
	 *  StartDeathSequence / DeactivateForPool). */
	void CancelAttackAbility();

	/** Health changed: sync the overhead bar and flash red on hits (damage
	 *  numbers come from the replicated OnDamaged result instead). */
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/** Damage resolved on this enemy (server): ships the result to every
	 *  viewer as a GameplayCue event (see HandleGameplayCue - one cue per
	 *  damage application, so same-frame hits all arrive). */
	UFUNCTION()
	void HandleDamaged(float FinalDamage, bool bCritical, bool bMissed, EDamageType DamageType);

	/** GameplayCueInterface: the damage-number cue routes here on the server
	 *  (local invoke) and on every client (multicast RPC with the full
	 *  parameters). Renders the number through the local HUD - the single
	 *  render path for host and clients alike. */
	virtual void HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters) override;

	/** Shows/hides the overhead status (component + widget level: the
	 *  component-level hide does not always cover Screen-space widgets). */
	void SetStatusWidgetVisible(bool bVisible);

	/** The local player's HUD (null on dedicated servers; each client gets its own view). */
	class ASnHUD* GetLocalSnHUD() const;

	/** Overlays the red material on the mesh, then restores after FlashDuration. */
	void FlashRed();
	void ClearFlash();

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<USnAttributeSet> AttributeSet;

	/** Overhead widget: level text (always) + health bar (combat only). */
	UPROPERTY(VisibleAnywhere, Category = "Enemy")
	TObjectPtr<UWidgetComponent> EnemyStatusComponent;

	/** Widget class for the overhead status (e.g. WBP_EnemyStatus). */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<UUserWidget> EnemyStatusWidgetClass;

	/** Row name (ID) in the global enemy data table (USnSettings) that defines this enemy's stats. */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	FName EnemyID;

	/** XP granted to the killer; filled from the data table at init. */
	UPROPERTY(BlueprintReadOnly, Category = "Enemy")
	int32 XPReward = 0;

	/** XP effect applied to the killer on death (e.g. GE_XP_Grant). */
	UPROPERTY(EditDefaultsOnly, Category = "Enemy")
	TSubclassOf<UGameplayEffect> XPGrantEffectClass;

	/** Applied on begin play to set up default attribute values (GE_InitAttributes). */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	/** Semi-transparent red material shown briefly on the mesh when hit. */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback")
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	/** How long the red flash stays before restoring the original material. */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback")
	float FlashDuration = 0.15f;

	/** Height above the actor where damage numbers spawn (upper body, not above the head). */
	UPROPERTY(EditDefaultsOnly, Category = "HitFeedback")
	float DamageNumberHeight = 90.f;

	/** XP shard actor spawned on death (e.g. BP_XPPickup). */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<ASnXPPickup> XPPickupClass;

	/** Item pickup actor spawned for table drops (e.g. BP_ItemPickup). */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	TSubclassOf<ASnItemPickup> ItemPickupClass;

	/** How many shards the XP reward is split into. */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	int32 XPShardCount = 4;

	/** Delay after death before the shards pop out (lets the body fall first). */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	float DropDelay = 0.1f;

	/** How long the fallen body stays before being destroyed (seconds). */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	float DeathDuration = 2.f;

	/** Fall rotation about the ACTOR's Y axis (pitch), in degrees - the mesh
	 *  tips over the actor's left-right axis when falling dead (-90 per spec).
	 *  Composed on the outside (Fall * pose), so the body always lies aligned
	 *  with the actor regardless of the blueprint mesh yaw. */
	UPROPERTY(EditDefaultsOnly, Category = "Loot")
	float DeathFallPitch = -90.f;

	/** Distance to sink a pooled enemy's hidden body below its death spot (the
	 *  corpse is otherwise left standing on the field while inactive). */
	UPROPERTY(EditDefaultsOnly, Category = "Pool")
	float PoolHideSinkDepth = 1000.f;

	/** Swing ability granted at init and activated by Tick when the chase
	 *  target is in range (e.g. GA_EnemyAttack). Unset = the enemy never
	 *  attacks - there is no damage path left in this class. */
	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<UGameplayAbility> AttackAbilityClass;

	/** Distance to the chase target at which the enemy swings. Only the start
	 *  of a swing is gated here: the ability's own cooldown paces the rest
	 *  (there is no interval timer). */
	UPROPERTY(EditDefaultsOnly, Category = "Attack", meta = (DisplayName = "攻击距离"))
	float AttackRange = 150.f;

	/** Distance to the player at which the enemy leaps at them. */
	UPROPERTY(EditDefaultsOnly, Category = "Leap", meta = (DisplayName = "起跳距离"))
	float JumpRange = 250.f;

	/** Horizontal leap force toward the player (vertical speed is the movement
	 *  component's JumpZVelocity - the "jump height" property). */
	UPROPERTY(EditDefaultsOnly, Category = "Leap", meta = (DisplayName = "跳跃水平力度"))
	float JumpForwardForce = 700.f;

	/** Seconds between leaps (the enemy must not jump constantly). */
	UPROPERTY(EditDefaultsOnly, Category = "Leap", meta = (DisplayName = "跳跃冷却（秒）"))
	float JumpCooldown = 1.5f;

private:
	FTimerHandle FlashTimerHandle;

	/** Drives the death fall interpolation. */
	FTimerHandle DeathFallTimerHandle;

	float DeathFallElapsed = 0.f;

	/** Fixed lying-down pose captured when the fall starts (the standing pose
	 *  composed with the local-X fall rotation). Each tick slerps toward it -
	 *  recomputing it per frame would keep adding rotation and spin the body. */
	FQuat DeathFallTarget = FQuat::Identity;

	/** Delays the shard drop until the death sequence starts. */
	FTimerHandle DropTimerHandle;

	/** True from death until the instance is reused; no attacking meanwhile. */
	bool bIsDying = false;

	/** The ability granted at init (AttackAbilityClass). Tick activates through
	 *  this handle and IsAttacking reads it; invalid while no class is
	 *  configured. Survives pooling - the spec is granted once and reused. */
	FGameplayAbilitySpecHandle AttackAbilityHandle;

	/** World time of the last leap (JumpCooldown gates the next one). */
	float LastJumpTime = 0.f;

	UPROPERTY()
	TObjectPtr<USnEnemyStatusWidget> EnemyStatusWidget;

	/** Level shown over the head; filled from the data table at init. */
	int32 Level = 1;

	/** Bumped by the server on every pool reuse (ConfigureForSpawn). Clients
	 *  restore their local-only presentation when it changes (OnRep): the death
	 *  fall and the hit flash are local visuals and never replicate, so a
	 *  reused enemy would otherwise spawn on clients still lying flat or
	 *  flashing red. */
	UFUNCTION()
	void OnRep_SpawnVisualToken();

	UPROPERTY(ReplicatedUsing = OnRep_SpawnVisualToken)
	int32 SpawnVisualToken = 0;
};
