// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SnPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Engine/World.h"
#include "Game/Save/SnSaveTypes.h"
#include "Game/SnGameState.h"
#include "Game/USnGameInstance.h"
#include "Inventory/USnInventoryComponent.h"
#include "GameplayEffect.h"
#include "Net/UnrealNetwork.h"
#include "ScalableFloat.h"

ASnPlayerState::ASnPlayerState()
{
	SetNetUpdateFrequency(100.f);

	// ASC and AttributeSet are both subobjects of this actor, so the attribute
	// set is automatically registered with the ASC.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<USnAttributeSet>(TEXT("AttributeSet"));

	InventoryComponent = CreateDefaultSubobject<USnInventoryComponent>(TEXT("InventoryComponent"));

	// Mark this ASC as the enemy attacks' valid target (Combat.Team.Player):
	// USnGA_EnemyAttack::IsValidAttackTarget decides by this tag, the mirror of
	// the enemy's own marker. Added once at construction on every machine that
	// builds this PlayerState (constructors run on clients too, so nothing has
	// to replicate), and never removed - the ASC lives for the whole session.
	// Uses AddLooseGameplayTag because the ASC is not initialized yet.
	AbilitySystemComponent->AddLooseGameplayTag(SnGameplayTags::Combat_Team_Player);
}

void ASnPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASnPlayerState, ProfileID);
}

UAbilitySystemComponent* ASnPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASnPlayerState::InitAbilityActorInfo(AActor* AvatarActor)
{
	if (!AbilitySystemComponent || !AvatarActor)
	{
		return;
	}

	// Already initialized correctly (a later readiness event pointed the ASC at
	// the same pawn - the client runs this from OnPlayerStateChanged and again
	// from OnRep_Controller, and only the first one has work to do).
	AActor* CurrentAvatar = AbilitySystemComponent->GetAvatarActor();
	if (CurrentAvatar == AvatarActor)
	{
		return;
	}

	// Register the default-subobject attribute set with the ASC up front (Aura
	// pattern). Without this, the first applied gameplay effect would lazily
	// create a second transient set via GetOrCreateAttributeSubobject - the ASC
	// would then replicate *that* one, leaving the PlayerState member an orphan.
	// Registering here makes the subobject the single replicated set: clients
	// have a non-empty SpawnedAttributes from construction (so GetSet<>() and
	// the UI never see null), and attribute value replication travels with it.
	// AddSpawnedAttribute is AddUnique - idempotent across repeat calls and
	// replication.
	AbilitySystemComponent->AddSpawnedAttribute(AttributeSet);

	// The engine may have auto-initialized the ASC on registration with the owning
	// actor as both owner and avatar (PlayerState, PlayerState). Point it at the
	// actual possessed pawn before applying anything.
	AbilitySystemComponent->InitAbilityActorInfo(this, AvatarActor);

	// Apply default attribute values (Health, MaxHealth, MP, PhysicalDamage).
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

	// Infinite MP regen, applied once per ASC lifetime (defaults above just
	// landed, so the AttributeBased magnitude reads the real MPRegen; respawns
	// re-enter this function and the flag stops a second stack).
	if (ManaRegenEffectClass && !bManaRegenApplied)
	{
		FGameplayEffectContextHandle ManaContext = AbilitySystemComponent->MakeEffectContext();
		ManaContext.AddSourceObject(this);
		const FGameplayEffectSpecHandle ManaSpec = AbilitySystemComponent->MakeOutgoingSpec(ManaRegenEffectClass, 1.f, ManaContext);
		if (ManaSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*ManaSpec.Data.Get());
			bManaRegenApplied = true;
		}
	}

	// Infinite health regeneration, same shape and same once-per-ASC rule. The
	// effect is a periodic one, so the guard matters doubly: a second instance
	// would tick the regen twice as fast rather than stack a passive.
	if (HealthRegenEffectClass && !bHealthRegenApplied)
	{
		FGameplayEffectContextHandle RegenContext = AbilitySystemComponent->MakeEffectContext();
		RegenContext.AddSourceObject(this);
		const FGameplayEffectSpecHandle RegenSpec = AbilitySystemComponent->MakeOutgoingSpec(HealthRegenEffectClass, 1.f, RegenContext);
		if (RegenSpec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
			bHealthRegenApplied = true;
		}
	}

	// Grant the player's abilities (runs once, after the avatar is corrected).
	if (AttackAbilityClass)
	{
		AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(AttackAbilityClass, 1));
	}

	// Watch XP so level-ups trigger (bound once: this function early-returns
	// on repeat calls with the same avatar, and the handle guards respawns).
	if (!XPChangeDelegateHandle.IsValid())
	{
		XPChangeDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetXPAttribute())
			.AddUObject(this, &ASnPlayerState::HandleXPChanged);
	}

	// From here on the state is a valid restore target: defaults applied,
	// abilities granted, ASC on the right avatar. This is what lets a late
	// profile report re-enter the restore (see SetProfileID) without racing the
	// setup above.
	bAbilitySystemInitialized = true;

	// Whatever was captured for this player (if anything) applies here: the
	// defaults above just landed, so the captured values replace them. Level is
	// restored before XP so the XP-change delegate's level-up check (bound
	// above) sees the saved level.
	TryRestoreFromSave();
}

void ASnPlayerState::FillCharacterSnapshot(FSnCharacterSave& OutCharacter) const
{
	if (InventoryComponent)
	{
		// Full-overwrite capture: the caller's buffer (the GameInstance's
		// per-player entry) can hold an earlier capture from a previous
		// save/travel - without the reset, equipment would accumulate duplicates
		// on every capture and the restore would re-apply each duplicate (GE
		// stacking, double summons).
		OutCharacter.Inventory.Reset();
		OutCharacter.EquipmentGrid.Reset();
		OutCharacter.HotbarSlots.Reset();

		OutCharacter.Inventory = InventoryComponent->GetInventory();
		// Grid records are pure logical state (row name + cell + rotation) -
		// no runtime handles, so they serialize directly; restore rebuilds
		// the equip GE/GA from the layout (RecomputeGridEffects).
		OutCharacter.EquipmentGrid = InventoryComponent->GetEquipmentGrid();

		// Hotbar slot bindings (row names, fixed order). The SELECTION is not
		// saved - it is a session choice; the slots themselves are layout.
		for (int32 SlotIndex = 0; SlotIndex < USnInventoryComponent::HotbarSize; ++SlotIndex)
		{
			OutCharacter.HotbarSlots.Add(InventoryComponent->GetHotbarSlot(SlotIndex));
		}

		// Wallet ledger (server-authoritative; the getter is the same value the
		// clients see through replication).
		OutCharacter.Gold = InventoryComponent->GetGold();
	}

	if (AbilitySystemComponent)
	{
		// BASE attributes (without GE modifiers): equip bonuses live in the
		// equipped GEs, which are re-applied on restore - snapshotting the
		// current (bonus-included) values here would bake the bonuses into the
		// base and double-apply them after the replay.
		OutCharacter.XP = AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetXPAttribute());
		OutCharacter.Level = FMath::Max(FMath::TruncToInt(AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetLevelAttribute())), 1);
		OutCharacter.Health = AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetHealthAttribute());
		OutCharacter.MaxHealth = AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetMaxHealthAttribute());
		// Shield is granted by Instant effects only (see USnAttributeSet), so
		// the whole pool sits in the base value - unlike the equip bonuses
		// above, there is nothing to re-apply after the restore.
		OutCharacter.Shield = AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetShieldAttribute());
		OutCharacter.AttackPower = AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetPhysicalDamageAttribute());
		OutCharacter.Capacity = AbilitySystemComponent->GetNumericAttributeBase(USnAttributeSet::GetCapacityAttribute());
	}

	// The profile is stamped by the save layer (USnGameInstance), not captured
	// here: the GameInstance is the one that knows which profile this machine's
	// player claims to be, and the same capture is also taken for guests, whose
	// IDs only the session key knows.
}

void ASnPlayerState::FillRunSnapshot(FSnRunSave& OutRun) const
{
	// Where the player stands now - restored on a continue-game entry only
	// (map-to-map travel reuses PlayerStart). GetPawn() is null in the main
	// menu, which never captures anyway (bInSession guards the save layer).
	if (const APawn* Pawn = GetPawn())
	{
		OutRun.SavedLocation = Pawn->GetActorLocation();
		OutRun.SavedRotation = Pawn->GetActorRotation();
	}

	// The run's difficulty tier: a new world's GameState starts at "Normal",
	// so the tier travels through the capture instead (restored in
	// TryRestoreFromSave). Worldless captures keep the previous value - the
	// menu never captures the run.
	if (const UWorld* World = GetWorld())
	{
		if (const ASnGameState* GameState = World->GetGameState<ASnGameState>())
		{
			OutRun.Difficulty = GameState->GetDifficulty();
		}
	}

	if (const UWorld* World = GetWorld())
	{
		// The level's name without its PIE prefix ("UEDPIE_0_HubMap" -> "HubMap"),
		// so the saved name opens correctly in PIE and in a packaged build (where
		// there is no prefix and the string passes through unchanged). The
		// engine's own normalizer, which is also what
		// USnBuildLibrary::GetBuildLevelName calls - a save and a build record
		// have to agree on what the current level is called.
		OutRun.LevelName = UWorld::RemovePIEPrefix(World->GetMapName());
	}
}

void ASnPlayerState::RestoreFromCharacter(const FSnCharacterSave& InCharacter)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// Level before XP: setting XP fires the change delegate (bound in
	// InitAbilityActorInfo), which runs CheckForLevelUp against the saved
	// level. MaxHealth/Health before XP too - a level-up mid-restore would
	// otherwise stack growth on top of the restored max.
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetLevelAttribute(), InCharacter.Level);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetMaxHealthAttribute(), InCharacter.MaxHealth);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetHealthAttribute(), FMath::Clamp(InCharacter.Health, 0.f, InCharacter.MaxHealth));
	// Shield sits in front of Health and lives purely in the base value, so it
	// round-trips directly; PreAttributeChange floors a stale negative at 0.
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetShieldAttribute(), InCharacter.Shield);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetPhysicalDamageAttribute(), InCharacter.AttackPower);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetCapacityAttribute(), InCharacter.Capacity);
	AbilitySystemComponent->SetNumericAttributeBase(USnAttributeSet::GetXPAttribute(), InCharacter.XP);

	if (InventoryComponent)
	{
		InventoryComponent->CopyStateFrom(InCharacter.Inventory, InCharacter.EquipmentGrid, InCharacter.Gold);
		// The ASC is fresh after a travel/continue: replay every grid item's
		// GE + ability set so removal still has valid handles (the capture
		// stores base attributes - the equip bonuses live in the GEs and must
		// be re-applied, not baked into the base). GrantEquipAbilities
		// activates the passive abilities right after the grant (grant +
		// activate in one, owned by the equip flow).
		InventoryComponent->ReapplyGridEffects();

		// Hotbar slot bindings (references - stale row names stay bound and
		// simply show as empty until the table has them again). The empty guard
		// covers a default-constructed character (nothing captured): the slots
		// stay untouched. Selection is intentionally not restored.
		// CopyHotbarFrom broadcasts OnHotbarChanged; any hotbar widgets/held
		// visual refresh from it.
		if (InCharacter.HotbarSlots.Num() > 0)
		{
			InventoryComponent->CopyHotbarFrom(InCharacter.HotbarSlots);
		}
	}
}

void ASnPlayerState::RestoreFromRun(const FSnRunSave& InRun)
{
	// Position: restored only on a continue-game entry - bRestoreLocation is
	// set by ContinueGame and cleared by PrepareServerTravel, so map-to-map
	// travel always reuses PlayerStart. The caller is authority-only: the pawn
	// spawns on the server and its transform replicates to clients. The
	// non-zero guard covers a run with nothing captured (a fresh run, or a
	// guest's empty file) - never teleport to the origin.
	if (const USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		if (GI->ShouldRestoreLocation() && !InRun.SavedLocation.IsNearlyZero())
		{
			if (APawn* Pawn = GetPawn())
			{
				Pawn->SetActorLocation(InRun.SavedLocation, /*bSweep=*/false);
				Pawn->SetActorRotation(InRun.SavedRotation);
			}
		}
	}

	// Session tier across worlds: a fresh GameState defaults to "Normal" - the
	// captured tier is restored here and the zone hook applies it to the player
	// later in RestartPlayer. SetDifficulty validates the row against the table
	// - a stale name (table changed since the save) is rejected and the run
	// stays "Normal".
	if (!InRun.Difficulty.IsNone())
	{
		if (UWorld* World = GetWorld())
		{
			if (ASnGameState* GameState = World->GetGameState<ASnGameState>())
			{
				GameState->SetDifficulty(InRun.Difficulty);
			}
		}
	}
}

void ASnPlayerState::TryRestoreFromSave()
{
	USnGameInstance* GI = GetGameInstance<USnGameInstance>();
	if (!GI)
	{
		return;
	}

	// Nothing to restore onto yet. A profile report can arrive before this
	// state's setup finished (SetProfileID calls in here too) and restoration
	// overwrites the defaults - running early would let the defaults be applied
	// over the captured character afterwards.
	if (!bAbilitySystemInitialized)
	{
		return;
	}

	// Server-only restore: clients never write their own copy of the
	// attributes/inventory - the server's replicated state is the only source
	// of truth, and a local restore would plant ghost items the server never
	// sees (every later Server RPC rejects them). Standalone is unaffected
	// (local = authority). On a listen server every PlayerState is authority,
	// which is exactly what lets each guest's own state take its character
	// back - the capture was taken here, on this machine, for all of them.
	if (!HasAuthority())
	{
		return;
	}

	// The character half, applied at most once per state: the roster entry is
	// read, not consumed (it is the room's record and every level switch reads
	// from it again), so the guard has to live here instead. A respawn re-enters
	// this function on the same state - applying twice would re-grant and
	// re-place everything the player already has - while a travel builds a new
	// state, which is exactly when the character does have to be applied again.
	// (Guarded, not returned: the run half below is independent of it.)
	if (!bCharacterRestored)
	{
		// A state with no entry keeps the defaults it just got: a guest who
		// joined mid-run has no history in this room (and neither has one whose
		// ID this room already had - see ServerSetPlayerName's duplicate check).
		FSnCharacterSave Character;
		if (GI->GetSessionCharacter(this, Character))
		{
			RestoreFromCharacter(Character);
			bCharacterRestored = true;
		}
	}

	// The run half - the new world's difficulty tier, and the continuing
	// player's position - is applied by THIS machine's own state only. The
	// guests' states initialize on this same server (non-seamless travel
	// rebuilds them all here) and one of them initializing first must not
	// consume the room's restore before the state it belongs to has.
	if (GI->IsLocalPlayerState(this))
	{
		FSnRunSave Run;
		if (GI->TakePendingRun(Run))
		{
			RestoreFromRun(Run);
		}
	}
}

void ASnPlayerState::SetProfileID(const FString& InProfileID)
{
	ProfileID = InProfileID;

	// Second chance to restore. The profile and this state's initialization are
	// two independent streams, and for a REMOTE player the profile necessarily
	// arrives last: the report is sent by that player's machine after it
	// possesses the pawn - and the pawn only exists because this server spawned
	// it, which is also the moment the first restore attempt ran (and found no
	// key). Without this retry a guest would lose their character on every
	// travel: their state initializes here without an ID to look up, and their
	// entry in the room's roster would go unread until the next travel captured
	// over it. Harmless in every other case - the lookup consumes nothing and
	// its guards handle a report that somehow beats the state's own setup.
	TryRestoreFromSave();
}

void ASnPlayerState::HandleXPChanged(const FOnAttributeChangeData& Data)
{
	CheckForLevelUp();
}

void ASnPlayerState::CheckForLevelUp()
{
	// Level-ups are a server-side decision: the client receives the result
	// through attribute replication (the UI's level display is driven by the
	// Level attribute change delegate, so it refreshes automatically). Re-
	// running the chain locally would apply the level-up effect a second time
	// and double-broadcast OnLevelChanged whenever XP replicates.
	if (!HasAuthority())
	{
		return;
	}

	if (!AbilitySystemComponent)
	{
		return;
	}

	int32 Level = FMath::Max(FMath::TruncToInt(AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetLevelAttribute())), 1);
	float XP = AbilitySystemComponent->GetNumericAttribute(USnAttributeSet::GetXPAttribute());

	float TotalHealthGain = 0.f;
	float TotalAttackGain = 0.f;
	int32 LevelsGained = 0;

	constexpr int32 MaxLevel = 100;
	while (Level < MaxLevel && XP >= USnAttributeSet::GetXPRequiredForLevel(Level))
	{
		XP -= USnAttributeSet::GetXPRequiredForLevel(Level);
		TotalHealthGain += LevelUpHealthGain;
		TotalAttackGain += LevelUpAttackGain;
		++Level;
		++LevelsGained;
	}

	if (LevelsGained == 0)
	{
		return;
	}

	// Apply the results in one runtime-constructed instant effect (the pattern
	// GASDocumentation uses for level-ups): Override XP/Level with the computed
	// values, Add the stat growth. Held by the member reference (LevelUpEffect)
	// so GC can never reap it mid-apply.
	UGameplayEffect* NewLevelUpEffect = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("LevelUp"));
	NewLevelUpEffect->DurationPolicy = EGameplayEffectDurationType::Instant;
	LevelUpEffect = NewLevelUpEffect;

	FGameplayModifierInfo XPModifier;
	XPModifier.Attribute = USnAttributeSet::GetXPAttribute();
	XPModifier.ModifierOp = EGameplayModOp::Override;
	XPModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(XP));
	NewLevelUpEffect->Modifiers.Add(XPModifier);

	FGameplayModifierInfo LevelModifier;
	LevelModifier.Attribute = USnAttributeSet::GetLevelAttribute();
	LevelModifier.ModifierOp = EGameplayModOp::Override;
	LevelModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Level));
	NewLevelUpEffect->Modifiers.Add(LevelModifier);

	FGameplayModifierInfo HealthGainModifier;
	HealthGainModifier.Attribute = USnAttributeSet::GetMaxHealthAttribute();
	HealthGainModifier.ModifierOp = EGameplayModOp::Type::AddBase;
	HealthGainModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(TotalHealthGain));
	NewLevelUpEffect->Modifiers.Add(HealthGainModifier);

	FGameplayModifierInfo HealModifier;
	HealModifier.Attribute = USnAttributeSet::GetHealthAttribute();
	HealModifier.ModifierOp = EGameplayModOp::Type::AddBase;
	HealModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(TotalHealthGain));
	NewLevelUpEffect->Modifiers.Add(HealModifier);

	FGameplayModifierInfo AttackGainModifier;
	AttackGainModifier.Attribute = USnAttributeSet::GetPhysicalDamageAttribute();
	AttackGainModifier.ModifierOp = EGameplayModOp::Type::AddBase;
	AttackGainModifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(TotalAttackGain));
	NewLevelUpEffect->Modifiers.Add(AttackGainModifier);

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	const FGameplayEffectSpec Spec(NewLevelUpEffect, Context, 1.f);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(Spec);

	OnLevelChanged.Broadcast(Level);
}
