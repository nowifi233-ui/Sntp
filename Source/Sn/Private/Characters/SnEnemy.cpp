// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/SnEnemy.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Characters/SnEnemyAIController.h"
#include "Loot/SnXPPickup.h"
#include "Loot/ASnItemPickup.h"
#include "Inventory/USnItemLibrary.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/World.h"
#include "Game/SnHUD.h"
#include "Combat/USnCombatLibrary.h"
#include "Spawn/USnEnemyPoolSubsystem.h"
#include "Loot/USnPickupSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Game/SnSettings.h"
#include "UI/Widget/SnEnemyStatusWidget.h"

ASnEnemy::ASnEnemy()
{
	// ASC and AttributeSet as character subobjects: the set is registered
	// automatically, and InitAbilityActorInfo(self, self) wires everything up.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<USnAttributeSet>(TEXT("AttributeSet"));

	// Mark this pawn as hostile (Combat.Team.Enemy on the ASC): combat queries
	// (USnCombatLibrary::IsEnemy) decide by this tag, not by class. Added once
	// at construction, never removed - pooling reuses the same ASC. Uses
	// AddLooseGameplayTag because the ASC is not initialized yet.
	AbilitySystemComponent->AddLooseGameplayTag(SnGameplayTags::Combat_Team_Enemy);

	// Overhead status: level text + combat health bar. Screen space so it
	// never rotates with the pawn.
	EnemyStatusComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("EnemyStatusComponent"));
	EnemyStatusComponent->SetupAttachment(GetMesh());
	EnemyStatusComponent->SetWidgetSpace(EWidgetSpace::Screen);
	EnemyStatusComponent->SetRelativeLocation(FVector(0.f, 0.f, 180.f));
	EnemyStatusComponent->SetDrawSize(FVector2D(250.f, 60.f));

	// Force the widget to tick every frame: the health bar animates its ghost
	// (delayed damage) bar in NativeTick, and WidgetComponent's default
	// ETickMode::Automatic stops ticking the widget while no UMG animation is
	// playing - which would freeze the ghost. The community-standard fix for
	// overhead health bars.
	EnemyStatusComponent->SetTickMode(ETickMode::Enabled);

	// Placed enemies automatically get the chase AI controller.
	AIControllerClass = ASnEnemyAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Overlap other pawns instead of blocking them: a blocking capsule would
	// stall the chase ~1m in front of the player (the enemy can never reach
	// its bite range). Enemies also pass through each other - survival-style.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// The camera arm's probe must pass through enemies (it shortens only
	// against real world geometry) - a blocking response here would yank the
	// camera in close every time an enemy stands behind the player.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	// Contact damage judgement runs every frame (deactivated while pooled).
	PrimaryActorTick.bCanEverTick = true;
}

UAbilitySystemComponent* ASnEnemy::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASnEnemy::BeginPlay()
{
	Super::BeginPlay();

	// The widget class comes from the BP defaults, so set it at runtime.
	if (EnemyStatusComponent && EnemyStatusWidgetClass)
	{
		EnemyStatusComponent->SetWidgetClass(EnemyStatusWidgetClass);
		EnemyStatusComponent->InitWidget();
		EnemyStatusWidget = Cast<USnEnemyStatusWidget>(EnemyStatusComponent->GetUserWidgetObject());
	}

	InitAbilityActorInfo();

	// Push the initial values once the level is known; the bar starts hidden
	// (combat mode decides when to show it).
	if (EnemyStatusWidget)
	{
		EnemyStatusWidget->SetLevel(Level);
		EnemyStatusWidget->SetHealthBarVisible(false);
		if (AbilitySystemComponent)
		{
			const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetMaxHealthAttribute());
			const float Health = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetHealthAttribute());
			EnemyStatusWidget->UpdateHealthPercent(MaxHealth > 0.f ? Health / MaxHealth : 0.f);
		}
	}
}

void ASnEnemy::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// Grant the attack ability once: the tick below activates it through this
	// handle for the rest of the actor's life. Pooled enemies re-run this init
	// on every reuse, so the handle guard matters - a second grant would orphan
	// the first spec, and the stale handle would point at an ability nothing
	// activates. Level 1: enemy scaling is the attribute multipliers' job
	// (ApplyDifficultyMultiplier), not the ability level's.
	if (AttackAbilityClass && !AttackAbilityHandle.IsValid())
	{
		AttackAbilityHandle = AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass, 1));
	}

	// The data table row is the source of truth for enemy base stats; fall
	// back to the GE-based init when no row is configured.
	if (const FSnEnemyInfo* Info = FindEnemyInfo())
	{
		ApplyEnemyInfo(*Info);
		BindAttributeCallbacks();
		return;
	}

	if (DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}

	BindAttributeCallbacks();
}

const FSnEnemyInfo* ASnEnemy::FindEnemyInfo() const
{
	UDataTable* Table = USnSettings::Get()->GetEnemyDataTable();
	return Table ? Table->FindRow<FSnEnemyInfo>(EnemyID, TEXT("SnEnemy::FindEnemyInfo")) : nullptr;
}

void ASnEnemy::ApplyEnemyInfo(const FSnEnemyInfo& Info)
{
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetMaxHealthAttribute(), Info.MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetHealthAttribute(), Info.MaxHealth);
	// The table's AttackPower column seeds the new PhysicalDamage attribute
	// (the numbers-system rename). MoveSpeed is seeded into the attribute too -
	// it is the single source of truth, and USnAttributeSet::SyncMoveSpeed
	// pushes any later GE-based change onto MaxWalkSpeed.
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetPhysicalDamageAttribute(), Info.AttackPower);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetMoveSpeedAttribute(), Info.MoveSpeed);

	XPReward = Info.XPReward;
	Level = Info.Level;

	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = Info.MoveSpeed;
	}
}

void ASnEnemy::BindAttributeCallbacks()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Idempotent: pooled enemies re-run init on every reuse. Unbind first so
	// each delegate fires exactly once per state change.
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetHealthAttribute())
		.RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ASnEnemy::HandleHealthChanged);

	if (AttributeSet)
	{
		AttributeSet->OnDeath.RemoveDynamic(this, &ASnEnemy::HandleDeath);
		AttributeSet->OnDeath.AddDynamic(this, &ASnEnemy::HandleDeath);

		AttributeSet->OnDamaged.RemoveDynamic(this, &ASnEnemy::HandleDamaged);
		AttributeSet->OnDamaged.AddDynamic(this, &ASnEnemy::HandleDamaged);
	}
}

void ASnEnemy::HandleDeath(AActor* Killer)
{
	// No attacking while dying (or after, until the instance is reused).
	bIsDying = true;

	// Announce the actual death (Blueprint special logic hooks here). Note this
	// is the attribute set's OnDeath call chain - keep it light, and never
	// Destroy this actor from it (the pool owns the recycling).
	OnEnemyDeath.Broadcast(Killer);

	// End the swing right here: a dying enemy must not land a pending hit frame
	// (the montage's Event.AttackWindow notify resolves the damage body), and
	// the client copies get the montage stop replicated. StartDeathFall covers
	// the client-side copy of that stop.
	CancelAttackAbility();

	// Loot pops out shortly after death (the body has started falling by then):
	// both the XP shards and the table item drops, on the same delay.
	GetWorldTimerManager().SetTimer(DropTimerHandle, this, &ASnEnemy::SpawnDropLoot, DropDelay, false);
	StartDeathSequence();
}

void ASnEnemy::SpawnDropLoot()
{
	SpawnXPPickups();
	SpawnItemPickups();
}

void ASnEnemy::SpawnXPPickups()
{
	if (XPReward <= 0 || !XPPickupClass)
	{
		return;
	}

	// Split the reward into shards; the last shard takes the remainder so the
	// total collected always equals the table's XPReward exactly.
	const int32 Count = FMath::Max(1, XPShardCount);
	const float BaseShard = static_cast<float>(XPReward) / Count;

	for (int32 i = 0; i < Count; ++i)
	{
		const float ShardValue = (i == Count - 1) ? XPReward - BaseShard * (Count - 1) : BaseShard;

		// Scatter around the upper body so shards pop out rather than stacking.
		const FVector SpawnLocation = GetActorLocation() + FVector(
			FMath::FRandRange(-50.f, 50.f),
			FMath::FRandRange(-50.f, 50.f),
			80.f);

		// Through the pickup subsystem: pooled shards are recycled instead of
		// spawned, and the field cap + lifetime keep the world from piling up.
		if (USnPickupSubsystem* PickupSystem = GetWorld()->GetSubsystem<USnPickupSubsystem>())
		{
			PickupSystem->SpawnXPPickup(XPPickupClass, SpawnLocation, GetActorRotation(), ShardValue, XPGrantEffectClass);
		}
	}
}

void ASnEnemy::SpawnItemPickups()
{
	if (!ItemPickupClass)
	{
		return;
	}

	const FSnEnemyInfo* Info = FindEnemyInfo();
	if (!Info)
	{
		return;
	}

	for (const FSnDropEntry& Entry : Info->Drops)
	{
		if (!USnItemLibrary::IsValidItem(Entry.ItemRowName) || Entry.MaxCount < 1)
		{
			continue;
		}
		if (FMath::FRand() > Entry.Chance)
		{
			continue;
		}

		const int32 DropCount = FMath::RandRange(FMath::Max(1, Entry.MinCount), Entry.MaxCount);

		// Scatter around the upper body, same feel as the XP shards.
		const FVector SpawnLocation = GetActorLocation() + FVector(
			FMath::FRandRange(-50.f, 50.f),
			FMath::FRandRange(-50.f, 50.f),
			80.f);

		// Through the pickup subsystem (same pooling + field management as the
		// XP shards above).
		if (USnPickupSubsystem* PickupSystem = GetWorld()->GetSubsystem<USnPickupSubsystem>())
		{
			PickupSystem->SpawnItemPickup(ItemPickupClass, SpawnLocation, GetActorRotation(), Entry.ItemRowName, DropCount);
		}
	}
}

void ASnEnemy::StartDeathSequence()
{
	// Keep the hit-flash material on for the whole death sequence; cancel the
	// short flash timer that would otherwise clear it.
	if (HitFlashMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(HitFlashMaterial);
	}
	GetWorldTimerManager().ClearTimer(FlashTimerHandle);

	// Freeze the body: no movement, no collisions, no AI chasing. StopMovement
	// first clears any leftover leap velocity so the body cannot drift down.
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->StopMovementImmediately();
		Movement->DisableMovement();
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	if (AController* EnemyController = GetController())
	{
		EnemyController->StopMovement();
		EnemyController->Destroy();
	}

	// Land the body: an enemy killed mid-leap would otherwise hang in the air
	// for the whole fall sequence. Snap down onto the ground under it, at
	// standing height - a body that died on the ground keeps its position
	// (it is already at ground + capsule half height).
	if (GetWorld())
	{
		const FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, 1000.f);
		const FVector TraceEnd = GetActorLocation() - FVector(0.f, 0.f, 2000.f);
		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic))
		{
			SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y,
				Hit.ImpactPoint.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() - 10.f));
		}
	}

	// Fall from upright to lying down. The mesh fall is a local-only transform
	// (it never replicates), so the server plays it and multicasts the
	// EnemyDeathFall cue for every client to replay locally.
	StartDeathFall();
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->ExecuteGameplayCue(SnGameplayTags::GameplayCue_EnemyDeathFall, FGameplayCueParameters());
	}
}

void ASnEnemy::StartDeathFall()
{
	// The overhead level/health widget follows the mesh - hide it while falling.
	SetStatusWidgetVisible(false);

	// The swing is already cancelled on the server (HandleDeath), but a client
	// copy replays this fall from the death cue and must not have a swing
	// montage fighting the fall pose on its mesh - the cancel's replication and
	// the cue are independent messages. Only stops a playing montage, so it is
	// a no-op wherever the animation is already down.
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->CurrentMontageStop();
	}

	// The lying pose is fixed ONCE here: the fall rotation composed on the
	// OUTSIDE of the standing pose (Fall * pose), so the mesh tips over the
	// ACTOR's Y axis - the body always lies aligned with the actor, regardless
	// of the blueprint mesh yaw. Inside composition (pose * Fall) would tip
	// over the model's own axis, leaving the body rotated relative to the
	// actor.
	if (GetMesh())
	{
		DeathFallTarget = FQuat(FRotator(DeathFallPitch, 0.f, 0.f))
			* GetMesh()->GetRelativeRotation().Quaternion();
	}
	DeathFallElapsed = 0.f;
	GetWorldTimerManager().SetTimer(DeathFallTimerHandle, this, &ASnEnemy::HandleDeathFallTick, 0.01f, true);
}

void ASnEnemy::SetStatusWidgetVisible(bool bVisible)
{
	if (EnemyStatusComponent)
	{
		EnemyStatusComponent->SetHiddenInGame(!bVisible);
	}
	if (EnemyStatusWidget)
	{
		// Component-level hiding does not always cover Screen-space widgets
		// (they draw through Slate), so collapse the widget itself too.
		EnemyStatusWidget->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void ASnEnemy::HandleDeathFallTick()
{
	// The timer fires at a fixed 0.01s interval, but the actual frame step
	// varies: integrate real deltas so the fall lasts DeathDuration in wall
	// clock time at any frame rate (a fixed step would stretch the fall to
	// 2-3x at low FPS, holding the pool slot hostage).
	DeathFallElapsed += GetWorld()->GetDeltaSeconds();
	const float Alpha = FMath::Clamp(DeathFallElapsed / FMath::Max(DeathDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);

	if (GetMesh())
	{
		const FQuat Start = GetMesh()->GetRelativeRotation().Quaternion();
		// Slerp toward the fixed target captured at fall start. Never compose
		// the fall onto the per-frame pose here: Slerp(q, q*Fall, a) = q*Fall^a
		// re-adds the rotation every tick and the body spins up.
		GetMesh()->SetRelativeRotation(FQuat::Slerp(Start, DeathFallTarget, Alpha));
	}

	if (Alpha >= 1.f)
	{
		GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);

		if (GetLocalRole() != ROLE_Authority)
		{
			// Client copy: the fall is done. Only the server recycles the actor
			// (its death sequence owns the pool slot and the OnEnemyDied
			// broadcast); the copy just waits for the server's recycle to hide
			// and destroy it.
			return;
		}

		// Trackers drop their reference, then the enemy recycles itself: the
		// pool deactivates it for reuse (or destroys it when full). No one has
		// to listen for this - the enemy decides its own fate.
		OnEnemyDied.Broadcast(this);
		if (USnEnemyPoolSubsystem* Pool = GetWorld()->GetSubsystem<USnEnemyPoolSubsystem>())
		{
			Pool->ReturnToPool(this);
		}
		else
		{
			Destroy();
		}
	}
}

void ASnEnemy::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// The bar always tracks the value (covers healing too).
	if (EnemyStatusWidget && AbilitySystemComponent)
	{
		const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetMaxHealthAttribute());
		EnemyStatusWidget->UpdateHealthPercent(MaxHealth > 0.f ? Data.NewValue / MaxHealth : 0.f);
	}

	if (Data.NewValue < Data.OldValue)
	{
		FlashRed();
	}
}

void ASnEnemy::HandleDamaged(float FinalDamage, bool bCritical, bool bMissed, EDamageType DamageType)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Ship the hit result as a GameplayCue event: ExecuteGameplayCue invokes
	// locally (standalone / listen host) and multicasts to every client as a
	// netmulticast RPC carrying the full parameters - each hit is an
	// independent event, so two damage applications in the same frame (base
	// hit + enchantment bonus) both arrive instead of one result overwriting
	// the other before replication (the old single replicated payload's bug).
	// The element and the crit/miss flags ride on the effect context, which
	// already replicates (FSnGameplayEffectContext::NetSerialize).
	FSnGameplayEffectContext* Context = new FSnGameplayEffectContext();
	Context->SetDamageType(DamageType);
	Context->SetIsCriticalHit(bCritical);
	Context->SetIsMissed(bMissed);

	FGameplayCueParameters Params;
	Params.RawMagnitude = FinalDamage;
	Params.EffectContext = FGameplayEffectContextHandle(Context);
	AbilitySystemComponent->ExecuteGameplayCue(SnGameplayTags::GameplayCue_DamageNumber, Params);
}

void ASnEnemy::HandleGameplayCue(UObject* Self, FGameplayTag GameplayCueTag, EGameplayCueEvent::Type EventType, const FGameplayCueParameters& Parameters)
{
	if (EventType != EGameplayCueEvent::Executed)
	{
		return;
	}

	if (GameplayCueTag == SnGameplayTags::GameplayCue_EnemyDeathFall)
	{
		// The server runs the fall itself (StartDeathSequence -> StartDeathFall);
		// clients replay it locally when the multicast arrives. The local role
		// guard keeps the server's own multicast from double-starting it.
		if (GetLocalRole() != ROLE_Authority)
		{
			StartDeathFall();
		}
		return;
	}

	if (GameplayCueTag != SnGameplayTags::GameplayCue_DamageNumber)
	{
		return;
	}

	// Damage numbers are a local-view effect: the HUD is null on dedicated
	// servers and each client renders its own (screen-space, not attached).
	ASnHUD* SnHUD = GetLocalSnHUD();
	if (!SnHUD)
	{
		return;
	}

	const FSnGameplayEffectContext* SnContext = Parameters.EffectContext.IsValid()
		? static_cast<const FSnGameplayEffectContext*>(Parameters.EffectContext.Get())
		: nullptr;
	const bool bMissed = SnContext ? SnContext->IsMissed() : false;
	const bool bCritical = SnContext ? SnContext->IsCriticalHit() : false;
	const EDamageType DamageType = SnContext ? SnContext->GetDamageType() : EDamageType::Physical;

	const FVector NumberLocation = GetActorLocation() + FVector(0.f, 0.f, DamageNumberHeight);
	if (bMissed)
	{
		SnHUD->ShowMissNumber(NumberLocation);
	}
	else if (bCritical)
	{
		SnHUD->ShowCriticalDamageNumber(NumberLocation, Parameters.RawMagnitude, DamageType);
	}
	else
	{
		SnHUD->ShowDamageNumber(NumberLocation, Parameters.RawMagnitude, DamageType);
	}
}

void ASnEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASnEnemy, SpawnVisualToken);
}

void ASnEnemy::OnRep_SpawnVisualToken()
{
	// The server reused this pooled instance (or it just spawned). The death
	// fall replay and the hit flash are local-only visuals (transforms and
	// materials never replicate), so the client copy restores its own
	// presentation here: standing mesh pose, no flash overlay, status widget
	// back on, no leftover fall timer. No-op for a fresh spawn (already at the
	// CDO pose).
	GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);
	DeathFallElapsed = 0.f;

	if (GetMesh())
	{
		if (const ASnEnemy* CDO = GetClass()->GetDefaultObject<ASnEnemy>())
		{
			GetMesh()->SetRelativeRotation(CDO->GetMesh()->GetRelativeRotation());
		}
		GetMesh()->SetOverlayMaterial(nullptr);
	}
	SetStatusWidgetVisible(true);
}

void ASnEnemy::SetHealthBarVisible(bool bVisible)
{
	if (EnemyStatusWidget)
	{
		EnemyStatusWidget->SetHealthBarVisible(bVisible);
	}
}

ASnHUD* ASnEnemy::GetLocalSnHUD() const
{
	if (GetWorld() && GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			return Cast<ASnHUD>(PC->GetHUD());
		}
	}
	return nullptr;
}

void ASnEnemy::FlashRed()
{
	if (HitFlashMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(HitFlashMaterial);
		GetWorldTimerManager().SetTimer(FlashTimerHandle, this, &ASnEnemy::ClearFlash, FlashDuration, false);
	}
}

void ASnEnemy::ClearFlash()
{
	if (GetMesh())
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}
}

void ASnEnemy::ConfigureForSpawn(FName InEnemyID, float HealthMultiplier, float AttackMultiplier, float SpeedMultiplier)
{
	// Reset everything the death sequence left behind.
	bIsDying = false;
	DeathFallElapsed = 0.f;
	LastJumpTime = 0.f;
	GetWorldTimerManager().ClearTimer(FlashTimerHandle);
	GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);
	// The loot timer also survives pooling: Stop() can interrupt the death
	// sequence mid-fall, and the late fire would drop the loot a second time.
	GetWorldTimerManager().ClearTimer(DropTimerHandle);

	// Reused enemies must not carry last life's active effects (DOTs, stuns,
	// slows) into the next one: combined with the re-armed ResetDeath latch
	// below, a residual burn would kill the reused enemy all over again.
	// 5.7 has no RemoveAllActiveEffects on the ASC - a default-constructed
	// FGameplayEffectQuery carries no criteria, so Matches() is always true
	// and RemoveActiveEffects clears every active effect (-1 = all stacks).
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveActiveEffects(FGameplayEffectQuery());
	}

	if (AttributeSet)
	{
		// The attribute set's one-shot death flag survives pooling; re-arm it
		// so a reused enemy can die again - without this, health hits zero but
		// OnDeath is swallowed by !bIsDead and the enemy never dies.
		AttributeSet->ResetDeath();
	}

	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	// SetActorEnableCollision only rebuilds the physics filter from each
	// component's stored value - it can't restore the capsule the death
	// sequence turned off at the component level (SetCollisionEnabled
	// overwrites the stored value). Pooled enemies would otherwise come back
	// with no collision at all. Fresh spawns short-circuit: their capsule is
	// already QueryAndPhysics.
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	SetActorTickEnabled(true);

	// Restore the blueprint-authored mesh pose (the class default, e.g. a 90°
	// yaw to align the model): the death sequence slerps the mesh down to a
	// lying pose and recycling leaves it there - without this restore, pooled
	// enemies would spawn lying flat or sideways. Fresh spawns already carry
	// the CDO pose (SpawnActor copies it), so this is a no-op for them.
	if (GetMesh())
	{
		if (const ASnEnemy* CDO = GetClass()->GetDefaultObject<ASnEnemy>())
		{
			GetMesh()->SetRelativeRotation(CDO->GetMesh()->GetRelativeRotation());
		}
		GetMesh()->SetOverlayMaterial(nullptr);
	}
	SetStatusWidgetVisible(true);

	// The death sequence destroyed the AI controller; give it a fresh one
	// (freshly spawned instances already have theirs from AutoPossessAI).
	if (!GetController())
	{
		SpawnDefaultController();
	}

	// Death left MovementMode at MOVE_None (DisableMovement). Reused enemies
	// would otherwise keep it: no gravity, no collision settling, no chasing -
	// they spawn stuck where they are (e.g. clipped into the ground).
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->SetDefaultMovementMode();
	}

	// Settle on the ground explicitly: fresh spawns get a collision pass from
	// SpawnActor (AdjustIfPossible), reused instances only get a bSweep=false
	// teleport while their collision was disabled - without this explicit drop
	// they can fall through the floor on their first walking frame.
	if (GetWorld() && GetCapsuleComponent())
	{
		FCollisionQueryParams QueryParams(FName(TEXT("SnEnemySettle")), /*bTraceComplex=*/false, this);
		// Start the sweep above the capsule top: if the actor is already
		// clipped into the ground, a sweep from its own position would start
		// below the surface and miss it. From above, the capsule always meets
		// the floor's top face on the way down.
		const FVector TraceStart = GetActorLocation() + FVector(0.f, 0.f, 100.f);
		const FVector TraceEnd = GetActorLocation() - FVector(0.f, 0.f, 500.f);
		FHitResult FloorHit;
		if (GetWorld()->SweepSingleByChannel(FloorHit, TraceStart, TraceEnd, FQuat::Identity,
			ECC_WorldStatic,
			FCollisionShape::MakeCapsule(GetCapsuleComponent()->GetScaledCapsuleRadius(), GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),
			QueryParams))
		{
			const float GroundZ = FloorHit.ImpactPoint.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 1.f;
			SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundZ), /*bSweep=*/false);
		}
	}

	// Re-apply the data table row for the ID, then scale it for the wave.
	EnemyID = InEnemyID;
	InitAbilityActorInfo();
	ApplyDifficultyMultiplier(HealthMultiplier, AttackMultiplier, SpeedMultiplier);

	// Bump the spawn token so clients re-apply their local-only presentation
	// (see OnRep_SpawnVisualToken): the mesh pose and the flash overlay are
	// local transforms/materials - the death-fall replay left the client mesh
	// lying flat, and nothing replicated would ever stand it back up.
	++SpawnVisualToken;
}

void ASnEnemy::DeactivateForPool()
{
	// Stop everything: no tick, no timers, no collision, no visibility. State
	// resets here so a later ConfigureForSpawn starts from a clean slate.
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	bIsDying = false;
	DeathFallElapsed = 0.f;
	LastJumpTime = 0.f;

	// A recycled enemy comes back inert: Stop() can recycle one mid-swing, and
	// an instance left running would keep its montage on a hidden corpse (and
	// resolve a hit) after the enemy respawned somewhere else.
	CancelAttackAbility();

	GetWorldTimerManager().ClearTimer(FlashTimerHandle);
	GetWorldTimerManager().ClearTimer(DeathFallTimerHandle);
	// A pending loot drop is part of the death sequence - Stop() while the
	// body is still falling must not let it fire later (double drops).
	GetWorldTimerManager().ClearTimer(DropTimerHandle);

	if (GetMesh())
	{
		GetMesh()->SetRelativeRotation(FRotator::ZeroRotator);
		GetMesh()->SetOverlayMaterial(nullptr);
	}
	SetStatusWidgetVisible(false);

	// Sink the hidden body below the play field: without this the corpse is
	// left standing at its death spot (capsule at ground + half height) for the
	// whole time it sits pooled. Reuse teleports to the new spawn point first
	// (SpawnEnemy), so the sink never needs to be undone - it only keeps dead
	// bodies off the map while inactive. TeleportPhysics: no sweep needed, the
	// actor is inert here (tick/collision/timers are off) and never moves again
	// until ConfigureForSpawn repositions it.
	SetActorLocation(GetActorLocation() - FVector(0.f, 0.f, PoolHideSinkDepth),
		/*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
}

void ASnEnemy::ApplyDifficultyMultiplier(float HealthMultiplier, float AttackMultiplier, float SpeedMultiplier)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Scale from the table values InitAbilityActorInfo just applied, then top
	// the health bar back up. Health is set after MaxHealth so the final value
	// already reflects the multiplier.
	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetMaxHealthAttribute()) * HealthMultiplier;
	const float PhysicalDamage = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetPhysicalDamageAttribute()) * AttackMultiplier;
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetMaxHealthAttribute(), MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetPhysicalDamageAttribute(), PhysicalDamage);
	if (AttributeSet)
	{
		AttributeSet->SetHealth(MaxHealth);
	}

	// Speed scales from the MoveSpeed attribute (single source of truth) and
	// pushes the movement component; a later GE-based change re-syncs through
	// USnAttributeSet::SyncMoveSpeed.
	const float MoveSpeed = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetMoveSpeedAttribute()) * SpeedMultiplier;
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetMoveSpeedAttribute(), MoveSpeed);
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->MaxWalkSpeed = MoveSpeed;
	}

	if (EnemyStatusWidget)
	{
		EnemyStatusWidget->SetLevel(Level);
		EnemyStatusWidget->UpdateHealthPercent(1.f);
		EnemyStatusWidget->SetHealthBarVisible(false);
	}
}

void ASnEnemy::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Attacking is a server-side judgement (standalone games pass here).
	if (bIsDying || !GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	// Chase target is "who we fight": the AI controller picks the nearest
	// Player-tagged actor (works right after level travel, unlike a PC pawn
	// lookup), and both the leap and the attack follow it.
	ASnEnemyAIController* EnemyAI = Cast<ASnEnemyAIController>(GetController());
	AActor* Player = EnemyAI ? EnemyAI->GetCurrentTarget() : nullptr;
	if (!Player)
	{
		return;
	}

	const float Distance2D = FVector::Dist2D(GetActorLocation(), Player->GetActorLocation());

	// Leap at the player when in range: jump toward them instead of just
	// walking up. Gated by cooldown and by being on the ground, so the
	// enemy pounces rather than hopping in place - and never mid-swing (a
	// launch would slide the body out from under its own animation).
	if (!IsAttacking()
		&& Distance2D < JumpRange
		&& GetCharacterMovement()->IsMovingOnGround()
		&& GetWorld()->GetTimeSeconds() - LastJumpTime >= JumpCooldown)
	{
		LastJumpTime = GetWorld()->GetTimeSeconds();
		const FVector Direction = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		// Vertical speed comes from the movement component's JumpZVelocity (the
		// native "jump height" property, tweaked in the BP). A hardcoded force
		// here would ignore whatever the designer set there.
		LaunchCharacter(Direction * JumpForwardForce + FVector::UpVector * GetCharacterMovement()->JumpZVelocity,
			/*bXYOverride=*/true, /*bZOverride=*/true);
	}

	// Attack when the target is in range: activate the swing ability, which
	// owns the animation, the pacing and the damage - this is only the trigger,
	// exactly like the player's input path. The per-frame retry is safe and
	// silent by design: the ability's own cooldown makes the engine refuse the
	// activation until the next swing is due (CanActivateAbility), and an
	// active instance refuses a second one, so nothing here needs a timer.
	if (Distance2D < AttackRange && !IsAttacking() && AttackAbilityHandle.IsValid()
		&& AbilitySystemComponent->TryActivateAbility(AttackAbilityHandle))
	{
		// Plant the feet and face the target: the move request the AI issued
		// would otherwise drag the enemy forward through its own animation, and
		// the damage sphere sits in FRONT of the attacker - a stopped enemy
		// keeps whatever yaw it had. Nothing turns it again until the swing ends
		// (the AI stops re-pathing while IsAttacking), so the yaw set here is
		// the one the hit frame sees.
		if (EnemyAI)
		{
			EnemyAI->StopMovement();
		}
		const FVector ToTarget = (Player->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
		if (!ToTarget.IsNearlyZero())
		{
			SetActorRotation(ToTarget.Rotation());
		}
	}
}

bool ASnEnemy::IsAttacking() const
{
	// An active ability instance IS the swing: the instance lives exactly as
	// long as the animation (USnGA_EnemyAttack ends with its montage) and the
	// engine reports it on the spec.
	if (!AbilitySystemComponent || !AttackAbilityHandle.IsValid())
	{
		return false;
	}

	const FGameplayAbilitySpec* Spec = AbilitySystemComponent->FindAbilitySpecFromHandle(AttackAbilityHandle);
	return Spec && Spec->IsActive();
}

void ASnEnemy::CancelAttackAbility()
{
	if (!AbilitySystemComponent || !AttackAbilityHandle.IsValid())
	{
		return;
	}

	// Cancel rather than End: the ability is torn down as cancelled, its
	// montage task takes the animation down with it, and the server's montage
	// stop replicates to the client copies. A no-op when nothing is running, so
	// callers never need to check the state themselves.
	AbilitySystemComponent->CancelAbilityHandle(AttackAbilityHandle);
}
