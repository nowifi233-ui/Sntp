// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnAbilityCatalog.h"
#include "AbilitySystem/SnAttributeCatalog.h"
#include "Engine/DataTable.h"
#include "Engine/DeveloperSettings.h"
#include "Engine/World.h" // TSoftObjectPtr<UWorld>
#include "Materials/MaterialInterface.h" // TSoftObjectPtr<UMaterialInterface>
#include "Misc/PackageName.h" // FPackageName::ObjectPathToPackageName
// Full include (not forward declaration): TSoftClassPtr::LoadSynchronous needs
// the complete class to validate the loaded UClass via IsChildOf.
#include "Loot/ASnItemPickup.h"
#include "GameplayEffect.h"
#include "SnSettings.generated.h"

/** One difficulty tier (row name = the tier ID, e.g. "Easy" / "Normal" /
 *  "Hard"). Multipliers above 1 help the player, below 1 weaken the player;
 *  the enemy multiplier scales the other way (tougher enemies on hard).
 *  The player-side multipliers are applied as a runtime-constructed Infinite
 *  GE while inside a combat zone (and removed back in the hub). */
USTRUCT(BlueprintType)
struct FSnDifficultyRow : public FTableRowBase
{
	GENERATED_BODY()

	/** The tier's ID (the table row name), filled at runtime by the library
	 *  lookups (GetAllDifficultyRows). Not a table column - no EditAnywhere,
	 *  so it cannot be misconfigured - but Blueprints read it to call
	 *  SnPlayerController::SetDifficulty. Same pattern as
	 *  FSnCraftRecipeRow::RowName. */
	UPROPERTY(BlueprintReadOnly, Category = "Difficulty")
	FName RowName;

	/** Shown in the spawner menu (e.g. "简单 / 普通 / 困难"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	FText DisplayName;

	/** Enemy stat multiplier (health/attack/speed) applied by the spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float EnemyMultiplier = 1.f;

	/** Player MaxHealth multiplier. Health gets the same factor so the current
	 *  health ratio survives the switch. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float PlayerMaxHealthMult = 1.f;

	/** Player PhysicalDamage multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float PlayerAttackMult = 1.f;

	/** Player MoveSpeed multiplier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty")
	float PlayerMoveSpeedMult = 1.f;
};

/**
 * Project settings entry (Edit > Project Settings > Sn Settings).
 * Points at the attribute catalog data asset, so the asset's location is
 * configured in one place instead of per-widget.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Sn Settings"))
class SN_API USnSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const USnSettings* Get() { return GetDefault<USnSettings>(); }

	UFUNCTION(BlueprintPure, Category = "SnSettings")
	USnAttributeCatalog* GetAttributeCatalog() const { return AttributeCatalog.LoadSynchronous(); }

	/** The global ability catalog. Each entry (FSnAbilityInfo) describes one
	 *  ability: identity tag / class / icon / name (descriptions are generated
	 *  by the ability itself via USnGameplayAbility::GetDescription). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	USnAbilityCatalog* GetAbilityCatalog() const { return AbilityCatalog.LoadSynchronous(); }

	/** The global enemy data table. Each row (FSnEnemyInfo) is one enemy type. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetEnemyDataTable() const { return EnemyDataTable.LoadSynchronous(); }

	/** The global difficulty data table. Each row (FSnDifficultyRow) is one
	 *  difficulty tier: selected from the spawner menu, stored on the player
	 *  state (server-authoritative), applied to the player while in a combat
	 *  zone and read by the spawner for enemy scaling. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetDifficultyDataTable() const { return DifficultyDataTable.LoadSynchronous(); }

	/** The global interaction data table. Each row (FSnInteractionRow) is one interactable type. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetInteractionDataTable() const { return InteractionDataTable.LoadSynchronous(); }

	/** The global dialogue data table. Each row (FSnDialogueRow) is one line
	 *  of a dialogue script; lines chain through NextRow (empty row name = the
	 *  dialogue ends). The first row's name doubles as the script ID -
	 *  triggers play it by name (SnPlayerController::PlayDialogue). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetDialogueDataTable() const { return DialogueDataTable.LoadSynchronous(); }

	/** How fast dialogue lines type themselves out (see USnDialogueWindow).
	 *  0 = typewriter off, every line appears at once. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	float GetDialogueCharsPerSecond() const { return DialogueCharsPerSecond; }

	/** The global item data table. Each row (FSnItemRow) is one item type. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetItemDataTable() const { return ItemDataTable.LoadSynchronous(); }

	/** The global item action data table. Each row (FSnItemActionRow) is one
	 *  right-click context menu action. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetItemActionDataTable() const { return ItemActionDataTable.LoadSynchronous(); }

	/** The global crafting recipe data table. Each row (FSnCraftRecipeRow) is
	 *  one recipe (materials in, item out). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetCraftRecipeDataTable() const { return CraftRecipeDataTable.LoadSynchronous(); }

	/** The global rune shape data table. Each row (FSnRuneShapeRow) is one
	 *  boost pattern; FSnItemRow::RuneShapeID references it by row name. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetRuneDataTable() const { return RuneDataTable.LoadSynchronous(); }

	/** The global quality style data table. Each row (FSnQualityStyleRow) is
	 *  one quality tier's UI colors/background, keyed by the enum name
	 *  (Common / Uncommon / ...) - the inventory/detail UI reads it by row
	 *  name. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetQualityStyleDataTable() const { return QualityStyleDataTable.LoadSynchronous(); }

	UDataTable* GetBondDataTable() const { return BondDataTable.LoadSynchronous(); }

	/** Hard cap on live + pooled enemies (USnEnemyPoolSubsystem). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	int32 GetEnemyPoolSize() const { return EnemyPoolSize; }

	/** The K constant of the armor curve (damage multiplier = K / (K + Defense)).
	 *  With K = 100 each point of Defense is worth +1% effective health: 100
	 *  halves incoming damage, 300 leaves a quarter of it. Read by
	 *  USnExecCalc_Damage - see the property for the tuning rationale. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	float GetDefenseConstant() const { return DefenseConstant; }

	/** Item pickup class used for discard drops (usually BP_ItemPickup, so
	 *  drops share the enemy loot visuals). Null = ASnItemPickup itself. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	TSubclassOf<ASnItemPickup> GetItemPickupClass() const { return ItemPickupClass.LoadSynchronous(); }

	/** The shared equip effect for ALL equipment: an Infinite GE whose
	 *  modifiers are all SetByCaller - the item row feeds the values on equip.
	 *  One asset instead of a GE per item. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	TSubclassOf<UGameplayEffect> GetGenericEquipEffectClass() const { return GenericEquipEffectClass.LoadSynchronous(); }

	/** The shared Instant effect for consumables (no duration). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	TSubclassOf<UGameplayEffect> GetGenericUseEffectClass() const { return GenericUseEffectClass.LoadSynchronous(); }

	/** The shared duration effect for consumables with UseDurationSeconds > 0
	 *  (Has Duration policy; the item row supplies the length at use time). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	TSubclassOf<UGameplayEffect> GetGenericBuffEffectClass() const { return GenericBuffEffectClass.LoadSynchronous(); }

	/** The shared item-use cooldown effect (Has Duration policy): its Duration
	 *  magnitude is SetByCaller UseCD and it grants Cooldown.Use. Per-item
	 *  existence is tracked by handle in USnInventoryComponent, so this one
	 *  asset serves every item - the length always comes from the row. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	TSubclassOf<UGameplayEffect> GetUseCooldownEffectClass() const { return UseCooldownEffectClass.LoadSynchronous(); }

	/** Card material for hotbar items without a WorldMesh: two-sided (M_ItemCard)
	 *  with a "CardTexture" texture parameter the code fills from the item row's
	 *  icon - the same look as the item pickups on the ground. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UMaterialInterface* GetIconCardMaterial() const { return IconCardMaterial.LoadSynchronous(); }

	/** Card plane mesh for the held icon card (the same plane the item pickups
	 *  use). The card + its material are mirrored from the drops, so a held
	 *  item without a WorldMesh reads as the drop's icon card. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UStaticMesh* GetIconCardMesh() const { return IconCardMesh.LoadSynchronous(); }

	/** Per-class hard cap: live XP shards, live item drops, and each class's
	 *  pooled instances are capped independently at this size (XP shards and
	 *  item drops never compete for field or pool space). Spawning evicts the
	 *  oldest live pickup of the same type to make room, so a new drop is
	 *  never swallowed. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	int32 GetPickupPoolSize() const { return PickupPoolSize; }

	/** Pickups older than this are returned to their class pool by the cleanup
	 *  sweep (destroyed only when that pool is full). Pooled instances are
	 *  exempt (already inert, not rendered, not ticking). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	float GetPickupLifetime() const { return PickupLifetime; }

	/** How often the cleanup sweep runs - one batch timer for all pickups,
	 *  instead of a timer per pickup. Expiry is quantized to this interval:
	 *  a pickup lives between PickupLifetime and PickupLifetime + Interval,
	 *  depending on its drop phase relative to the sweep (a coarse round is
	 *  preferred over a per-second scan of the whole field). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	float GetPickupCleanupInterval() const { return PickupCleanupInterval; }

	/** Lead time before each cleanup round at which OnCleanupImminent fires
	 *  (bare heads-up, no payload). <= 0 or >= PickupCleanupInterval disables
	 *  the notice. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	float GetPickupCleanupNoticeSeconds() const { return PickupCleanupNoticeSeconds; }

	/** The global build-device data table. Each row (FSnBuildDeviceRow) is one
	 *  placeable hub device; saved placement records reference it by DeviceID. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UDataTable* GetBuildDeviceDataTable() const { return BuildDeviceDataTable.LoadSynchronous(); }

	/** Master material for the placement ghost ("Color" vector parameter) - see
	 *  the property. Null = ghosts keep the mesh's own materials, untinted. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	UMaterialInterface* GetBuildPreviewMaterial() const { return BuildPreviewMaterial.LoadSynchronous(); }

	/** Tint for "this spot would be accepted" - see the property. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	FLinearColor GetBuildPreviewValidColor() const { return BuildPreviewValidColor; }

	/** Tint for "the server would refuse this spot" - see the property. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	FLinearColor GetBuildPreviewInvalidColor() const { return BuildPreviewInvalidColor; }

	/** Number of save slots offered by the main menu (Save_1..Save_N). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	int32 GetSaveSlotCount() const { return SaveSlotCount; }

	/** Level the "new game" flow travels to (the hub). Package path without
	 *  the object suffix, e.g. "/Game/Maps/HubMap" - works in PIE and packaged
	 *  builds. Computed from the path string, so the asset need not be loaded. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	FString GetStartLevelName() const { return FPackageName::ObjectPathToPackageName(StartLevel.ToString()); }

	/** Level the in-game menu's "return to main menu" travels to. Package path
	 *  without the object suffix - same convention as GetStartLevelName. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	FString GetMainMenuLevelName() const { return FPackageName::ObjectPathToPackageName(MainMenuLevel.ToString()); }

	/** Title shown large on the travel loading screen (SSnLoadingScreen). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	FText GetLoadingScreenTitle() const { return LoadingScreenTitle; }

	/** Status line shown under the spinner on the loading screen. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	FText GetLoadingScreenText() const { return LoadingScreenText; }

	/** How long the loading screen stays up at minimum, in seconds. */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	float GetLoadingScreenMinimumSeconds() const { return LoadingScreenMinimumSeconds; }

	/** True when LevelName names the main menu level - the ONE implementation of
	 *  that question. The travel chain asks it in three places (RequestTravel
	 *  picks guest-vs-host with it, ClientPrepareTravel picks the
	 *  leave-the-session branch, ServerTravelNow clears bInSession) and the
	 *  callers do not agree on a notation: a blueprint hands in the bare map name
	 *  ("Map_1") while the config holds a package path, and under PIE one side
	 *  may carry a world prefix. Both sides are normalized before comparing, with
	 *  a short-name fallback for a bare map name.
	 *
	 *  Why it matters: a bare "LevelName == GetMainMenuLevelName()" answered NO
	 *  to "Map_1", so a guest's "back to the menu" fell through to the ordinary
	 *  whole-team travel - the host was dragged into the menu with it, and the
	 *  room was never dissolved (it stayed in LAN searches with nobody in it). */
	UFUNCTION(BlueprintPure, Category = "SnSettings")
	static bool IsMainMenuLevel(const FString& LevelName)
	{
		const USnSettings* Settings = Get();
		if (!Settings || LevelName.IsEmpty())
		{
			return false;
		}

		const FString MenuLevelName = Settings->GetMainMenuLevelName();
		if (MenuLevelName.IsEmpty())
		{
			return false;
		}

		if (NormalizeLevelName(LevelName).Equals(NormalizeLevelName(MenuLevelName), ESearchCase::IgnoreCase))
		{
			return true;
		}

		// A bare map name is what blueprints usually carry: the package paths do
		// not match, the short names still have to.
		return FPackageName::GetShortName(LevelName).Equals(FPackageName::GetShortName(MenuLevelName), ESearchCase::IgnoreCase);
	}

private:
	/** The notation both sides of a level-name comparison can agree on: no PIE
	 *  world prefix, no object suffix, no stray whitespace. */
	static FString NormalizeLevelName(const FString& InLevelName)
	{
		return FPackageName::ObjectPathToPackageName(UWorld::RemovePIEPrefix(InLevelName)).TrimStartAndEnd();
	}

protected:
	/** The attribute catalog used to resolve attribute rows and tag lookups.
	 *  Soft reference: config serializes to ini as a path string, hard object
	 *  references are not allowed on config properties. */
	UPROPERTY(config, EditAnywhere, Category = "Attributes", meta = (AllowedClasses = "/Script/Sn.SnAttributeCatalog"))
	TSoftObjectPtr<USnAttributeCatalog> AttributeCatalog;

	/** The ability catalog used to resolve granted abilities to UI rows
	 *  (icon / name / identity tag). Soft reference: config rules as above. */
	UPROPERTY(config, EditAnywhere, Category = "Abilities", meta = (AllowedClasses = "/Script/Sn.SnAbilityCatalog"))
	TSoftObjectPtr<USnAbilityCatalog> AbilityCatalog;

	/** One global table for all difficulty tiers; rows keyed by tier ID (Easy / Normal / Hard...). */
	UPROPERTY(config, EditAnywhere, Category = "Difficulty", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> DifficultyDataTable;

	/** One global table for all enemy types; enemies reference it by row name (EnemyID). */
	UPROPERTY(config, EditAnywhere, Category = "Enemies", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> EnemyDataTable;

	/** One global table for all interactable types; interactables reference it by row name. */
	UPROPERTY(config, EditAnywhere, Category = "Interactions", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> InteractionDataTable;

	/** One global table for all dialogue lines; rows keyed by line ID and
	 *  chained through NextRow (empty = the end). Triggers start a script by
	 *  its first row's name (e.g. "Tutorial.HubIntro"). */
	UPROPERTY(config, EditAnywhere, Category = "Dialogue", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> DialogueDataTable;

	/** Typewriter speed for dialogue lines, in characters per second. 30 is a
	 *  brisk tutorial pace, 15 is slow and deliberate, 60 is fast. 0 turns the
	 *  typewriter off (every line appears at once) - and a click mid-reveal
	 *  completes the line whatever this is set to. */
	UPROPERTY(config, EditAnywhere, Category = "Dialogue", meta = (ClampMin = "0.0"))
	float DialogueCharsPerSecond = 30.f;

	/** One global table for all item types; items reference it by row name (ItemID). */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> ItemDataTable;

	/** One global table for right-click context menu actions; the menu filters
	 *  it by the clicked item's type/source (USnItemLibrary::GetAvailableItemActions). */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> ItemActionDataTable;

	/** One global table for all crafting recipes; the crafting window lists it
	 *  by row name (recipe ID). */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> CraftRecipeDataTable;

	/** One global table for all rune shapes; rune items reference it by row
	 *  name (FSnItemRow::RuneShapeID). */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> RuneDataTable;

	/** One global table for quality UI styles (colors + background per
	 *  ESnItemQuality, row name = enum name); the inventory/detail UI reads it
	 *  by row name. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> QualityStyleDataTable;

	/** One global table for bond (羁绊) tiers; row name = the bond's
	 *  gameplay tag name (e.g. "Item.Bond.Fire"). Items carry BondTags; the
	 *  inventory recomputes the equipped counts into bond GE effects
	 *  (USnInventoryComponent::RecomputeBonds). */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> BondDataTable;

	/** Hard cap on live + pooled enemies; spawning stops at this many concurrent enemies. */
	UPROPERTY(config, EditAnywhere, Category = "Enemies")
	int32 EnemyPoolSize = 50;

	/** Item pickup class for discard drops; soft class reference because the
	 *  class lives in Content (config can't hold hard references). Unset =
	 *  the C++ base class ASnItemPickup. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Sn.SnItemPickup", MetaClass = "/Script/Sn.SnItemPickup"))
	TSoftClassPtr<ASnItemPickup> ItemPickupClass;

	/** Shared equip GE (Infinite, SetByCaller modifiers) - see
	 *  GetGenericEquipEffectClass. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect", MetaClass = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> GenericEquipEffectClass;

	/** Shared consumable GE (Instant, SetByCaller modifiers) - see
	 *  GetGenericUseEffectClass. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect", MetaClass = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> GenericUseEffectClass;

	/** Shared consumable GE (Has Duration, SetByCaller modifiers) - see
	 *  GetGenericBuffEffectClass. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect", MetaClass = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> GenericBuffEffectClass;

	/** Shared item-use cooldown GE (Has Duration, Duration magnitude =
	 *  SetByCaller UseCD, grants Cooldown.Use) - see
	 *  GetUseCooldownEffectClass. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect", MetaClass = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> UseCooldownEffectClass;

	/** Two-sided icon-card material ("CardTexture" texture parameter) for the
	 *  held icon card - see GetIconCardMaterial. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.MaterialInterface", MetaClass = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> IconCardMaterial;

	/** Card plane mesh for the held icon card - see GetIconCardMesh. */
	UPROPERTY(config, EditAnywhere, Category = "Items", meta = (AllowedClasses = "/Script/Engine.StaticMesh", MetaClass = "/Script/Engine.StaticMesh"))
	TSoftObjectPtr<UStaticMesh> IconCardMesh;

	/** Per-class hard cap on live pickups and pooled instances - see
	 *  GetPickupPoolSize. */
	UPROPERTY(config, EditAnywhere, Category = "Pickups")
	int32 PickupPoolSize = 60;

	/** Field lifetime for pickups (seconds) - see GetPickupLifetime. */
	UPROPERTY(config, EditAnywhere, Category = "Pickups")
	float PickupLifetime = 60.f;

	/** Cleanup sweep interval (seconds) - see GetPickupCleanupInterval. */
	UPROPERTY(config, EditAnywhere, Category = "Pickups")
	float PickupCleanupInterval = 60.f;

	/** Pre-round notice lead time (seconds) - see GetPickupCleanupNoticeSeconds. */
	UPROPERTY(config, EditAnywhere, Category = "Pickups")
	float PickupCleanupNoticeSeconds = 20.f;

	/** K in the armor curve (USnExecCalc_Damage): the victim's damage multiplier
	 *  is K / (K + effective Defense), so this one number decides how much a
	 *  point of Defense is worth. 100 is the League of Legends convention and
	 *  the easiest to read (Defense 100 = half damage, each point = +1% effective
	 *  health); Dota 2 runs the same curve at K ~= 16.7, WoW scales K with level.
	 *  Raising K makes defense weaker, lowering it makes defense stronger - the
	 *  shape never changes: mitigation climbs toward 100% and never reaches it. */
	UPROPERTY(config, EditAnywhere, Category = "Combat", meta = (ClampMin = "1.0"))
	float DefenseConstant = 100.f;

	/** One global table for all placeable hub devices; saved placement records
	 *  reference it by row name (DeviceID). Can be empty until the building
	 *  system ships - the save format is ready. */
	UPROPERTY(config, EditAnywhere, Category = "Save", meta = (AllowedClasses = "/Script/Engine.DataTable"))
	TSoftObjectPtr<UDataTable> BuildDeviceDataTable;

	/** Material the placement ghost is drawn with - a translucent
	 *  unlit/emissive master material exposing a vector parameter named
	 *  "Color". The ghost writes that parameter to tint itself green or red
	 *  (see the BuildPreviewValidColor/InvalidColor below). Unset = the device
	 *  row's own materials, untinted: the ghost then shows the placement
	 *  position but not whether that position is allowed. */
	UPROPERTY(config, EditAnywhere, Category = "Building", meta = (AllowedClasses = "/Script/Engine.MaterialInterface", MetaClass = "/Script/Engine.MaterialInterface"))
	TSoftObjectPtr<UMaterialInterface> BuildPreviewMaterial;

	/** "Color" written to BuildPreviewMaterial when the spot is placeable. */
	UPROPERTY(config, EditAnywhere, Category = "Building")
	FLinearColor BuildPreviewValidColor = FLinearColor(0.f, 1.f, 0.f, 0.45f);

	/** "Color" written when the spot would be refused (the server would bounce
	 *  the placement and the item would not be consumed). */
	UPROPERTY(config, EditAnywhere, Category = "Building")
	FLinearColor BuildPreviewInvalidColor = FLinearColor(1.f, 0.15f, 0.1f, 0.45f);

	/** Save slots shown by the main menu ("Save_1" .. "Save_N"). */
	UPROPERTY(config, EditAnywhere, Category = "Save")
	int32 SaveSlotCount = 4;

	/** Level the new-game flow starts in (the hub). */
	UPROPERTY(config, EditAnywhere, Category = "Save", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> StartLevel;

	/** Level the in-game menu's "return to main menu" travels to (the session
	 *  ends there - see USnGameInstance::ServerTravelNow). */
	UPROPERTY(config, EditAnywhere, Category = "Save", meta = (AllowedClasses = "/Script/Engine.World"))
	TSoftObjectPtr<UWorld> MainMenuLevel;

	/** FText rather than a string literal in code: the loading widget is raw
	 *  Slate (see SSnLoadingScreen), so it has no Blueprint to restyle - this
	 *  and the two below are where its content is edited. */
	UPROPERTY(config, EditAnywhere, Category = "Loading")
	FText LoadingScreenTitle = FText::FromString(TEXT("Sn"));

	/** Status line under the spinner - "Loading..." by default. */
	UPROPERTY(config, EditAnywhere, Category = "Loading")
	FText LoadingScreenText = FText::FromString(TEXT("Loading..."));

	/** Minimum time the loading screen stays up (seconds), enforced even when
	 *  the level is already loaded: without it a fast travel strobes the screen
	 *  for a couple of frames. 0 hides it the moment loading completes. The wait
	 *  is spent spinning on the game thread inside WaitForMovieToFinish, so keep
	 *  it short - it is a floor for the animation, not a forced pause. */
	UPROPERTY(config, EditAnywhere, Category = "Loading", meta = (ClampMin = "0.0"))
	float LoadingScreenMinimumSeconds = 0.5f;
};
