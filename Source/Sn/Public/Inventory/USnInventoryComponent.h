// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ActiveGameplayEffectHandle.h"
#include "AttributeSet.h"
#include "Components/ActorComponent.h"
#include "Delegates/IDelegateInstance.h" // FDelegateHandle
#include "Inventory/FSnItemTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "GameplayTagContainer.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "USnInventoryComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSnInventoryChangedDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnCapacityChangedDelegate, int32, NewCapacity);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSnGoldChangedDelegate, int64, NewGold, int64, Delta);

/**
 * Server-local bookkeeping for ONE occupied grid cell (never replicated,
 * never saved): the equip effect and granted ability handles currently
 * applied from the cell's item, plus the row name and level they were
 * applied at (the incremental replay's diff key). Keyed by cell number in
 * USnInventoryComponent::GridRuntime; cells not in the map have nothing
 * applied. Rewritten by every recompute that touches the cell (incremental
 * ReplayItems or the full RecomputeGridEffects).
 */
USTRUCT()
struct FSnGridRuntimeState
{
	GENERATED_BODY()

	/** The applied equip GE (one per grid item), or invalid when the item
	 *  applied no effect. */
	UPROPERTY()
	FActiveGameplayEffectHandle EquipEffect;

	/** Ability handles granted from the item's ability set (0..N per item). */
	UPROPERTY()
	TArray<FGameplayAbilitySpecHandle> Abilities;

	/** Row name the current effects were applied from (empty when the cell
	 *  was never applied). ReplayItems skips a cell when BOTH this and
	 *  AppliedLevel match the current item - identical inputs produce
	 *  identical effects, so the skip can never drift. */
	UPROPERTY()
	FName AppliedRowName;

	/** Effective level the current effects were applied at (0 = never). */
	UPROPERTY()
	int32 AppliedLevel = 0;
};

/**
 * Client-local use-cooldown mirror for ONE item row (never replicated, never
 * saved): the world-seconds end time plus the total duration the cooldown
 * started with. The total lets the fraction getter (GetLocalUseCooldownFraction)
 * return an exact ratio even when the entry is first read midway through the
 * cooldown - e.g. the HUD (and its hotbar) rebuilt after a level travel. Once
 * EndTime passes both getters lazily return 0; stale entries are bounded by
 * the item row count and replaced on the next use.
 */
USTRUCT()
struct FSnLocalItemCooldown
{
	GENERATED_BODY()

	/** World seconds when the cooldown ends (lazily expired). */
	UPROPERTY()
	float EndTime = 0.f;

	/** The cooldown's full length in seconds, as started. */
	UPROPERTY()
	float TotalDuration = 0.f;
};

class UAbilitySystemComponent;
struct FAbilityEndedData;

/**
 * The player's inventory + equipment, mounted on the PlayerState (alongside
 * the ASC, so it survives pawn death/respawn). Server-authoritative: gameplay
 * writes go through the Server* RPCs (or direct calls on the server), the
 * arrays replicate to clients, UI reads via the BlueprintPure getters.
 *
 * Equipment is a GRID (6x6, Sephilia-style) instead of fixed slots: the
 * player freely arranges items in the cells. Runes (Item.Type.Rune) placed on
 * the grid boost the cells around them (FSnRuneShapeRow, rotated per item);
 * the boosted CELL level adds to an item's row level, and the effective level
 * feeds the generic equip GE values and the ability-set GA growth.
 *
 * Grid state is derived-to-apply: FSnGridItem holds pure logical data (row
 * name + cell + rotation) and the runtime effects are rebuilt from the
 * layout after every mutation - levels can never drift from the arrangement.
 * Rebuilds are INCREMENTAL: ReplayItems scans the coverage closure of the
 * changed cells but rebuilds only the cells whose row name or effective
 * level actually changed (minimal impact set - diffed per cell against the
 * applied row/level recorded in GridRuntime, so unrelated passives like
 * summons are never torn down and rune-dense layouts cost O(changed), not
 * O(all runes)); the full RecomputeGridEffects runs only when the ASC is
 * fresh (level travel / continue-game). Effects/abilities are server-local
 * bookkeeping (never replicated, never saved).
 */
UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class SN_API USnInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USnInventoryComponent();

	// --- Server-executed core operations (pickups, drops, loot call these) ---

	/** Adds a stack, auto-stacking into existing slots first, then new slots.
	 *  Returns the count actually added (0 if the inventory was full). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	int32 AddItem(FName ItemRowName, int32 Count);

	/** Removes Count of the item across slots. Returns false if the inventory
	 *  didn't have enough (nothing is removed then). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	bool RemoveItem(FName ItemRowName, int32 Count);

	/** Removes Count of ItemRowName from the clicked slot (SlotIndex) when the
	 *  slot still holds it; falls back to the row-wide removal when the slot
	 *  is invalid or changed since the click (multiplayer races). UI-click
	 *  operations (place/use/drop/transfer) go through here so they target the
	 *  clicked group - not whatever the row-name path finds last. Broadcasts
	 *  OnInventoryChanged on success. */
	bool RemoveItemAt(FName ItemRowName, int32 SlotIndex, int32 Count);

	// --- Client-safe entry points (route through Server RPCs) ---

	/** Places the item from the clicked bag slot (or row-name fallback) into
	 *  the equipment grid cell GridIndex with the given rotation (mod 4).
	 *  Only equipment and runes can be placed; the cell must be empty. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void PlaceInGrid(FName ItemRowName, int32 BagSlotIndex, int32 GridIndex, int32 RotationSteps = 0) { ServerPlaceInGrid(ItemRowName, BagSlotIndex, GridIndex, RotationSteps); }

	/** Removes the grid item back into the bag (auto-stack, or the clicked bag
	 *  slot when specified - empty slot or same-item non-full stack only).
	 *  Rejects when the bag is full or when removing a capacity gear would
	 *  leave the bag over its post-removal capacity. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void RemoveFromGrid(int32 GridIndex, int32 TargetBagSlotIndex = -1) { ServerRemoveFromGrid(GridIndex, TargetBagSlotIndex); }

	/** Moves the item between grid cells (drag-drop): an empty target moves,
	 *  an occupied target swaps the two items. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void MoveInGrid(int32 FromGridIndex, int32 ToGridIndex) { ServerMoveInGrid(FromGridIndex, ToGridIndex); }

	/** Rotates the grid item RotationSteps clockwise (mod 4, added to the
	 *  current rotation) - runes change which cells they boost. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void RotateGridItem(int32 GridIndex, int32 RotationSteps) { ServerRotateGridItem(GridIndex, RotationSteps); }

	/** Uses a consumable: applies its use GE, consumes one if configured.
	 *  SlotIndex is the clicked grid slot (optional; -1 = INDEX_NONE = row-name).
	 *  Every use source (selected hotbar item, bag right-click, ...) funnels
	 *  through here: the local cooldown gate + UI mirror run on the calling
	 *  side, then the authoritative half (bag check, server cooldown gate,
	 *  apply/consume) goes through the use RPC. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void UseItem(FName ItemRowName, int32 SlotIndex = -1);

	/** Drops Count into the world as an item pickup (if the item allows it).
	 *  SlotIndex is the clicked grid slot (optional; -1 = INDEX_NONE = row-name). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void DropItem(FName ItemRowName, int32 Count, int32 SlotIndex = -1) { ServerDropItem(ItemRowName, Count, SlotIndex); }

	/** Swaps/merges two slots within the bag (drag-drop; server-executed).
	 *  Same row name merges into the target; any slot (including empty ones)
	 *  can be a drop target - items swap freely, so dropping onto an empty
	 *  slot places the item there. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void SwapStacks(int32 FromIndex, int32 ToIndex) { ServerSwapStacks(FromIndex, ToIndex); }

	/** One-click sort: merges stacks, re-packs in row-name order (server). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void SortInventory() { ServerSortInventory(); }

	/** Crafts Count copies of the recipe (server-executed, all-or-nothing):
	 *  validates the materials, then consumes them and adds the output.
	 *  Nothing changes on failure (missing materials, full bag, bad recipe). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Inventory")
	void CraftItem(FName RecipeRowName, int32 Count = 1) { ServerCraftItem(RecipeRowName, Count); }

	// -------------------------------------------------------------------------
	// Wallet (gold). Server-authoritative ledger: every write goes through the
	// server (AddGold is server-only, SellItem rides a Server RPC) and
	// replicates back. Gold lives HERE instead of in a component of its own
	// because selling is a bag operation - remove the items and credit the gold
	// in one server call with no partial-failure path to refund - and every
	// caller of item code (item actions, UI controllers, save, cheats) already
	// holds this component. Rates that MULTIPLY an award belong on the awarding
	// side (same pattern as ASnPlayerState::GetXPGetMultiplier), never on the
	// balance itself.
	// -------------------------------------------------------------------------

	/** The player's gold (personal wealth, not a shared team pool). */
	UFUNCTION(BlueprintPure, Category = "Sn|Wallet")
	int64 GetGold() const { return Gold; }

	/** Credits gold; a negative amount debits. SERVER ONLY: logs and returns
	 *  elsewhere (a client's copy is written by replication, never locally).
	 *  Clamps into [0, MaxGold] and broadcasts OnGoldChanged with the applied
	 *  delta. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Wallet")
	void AddGold(int64 Amount);

	/** Sells Count of the item from the clicked bag slot: re-resolves the price
	 *  server-side, removes the items, credits the gold. Nothing changes when
	 *  the row has no sell price or the slot no longer holds that many (the shop
	 *  list can go stale while it stays open). SlotIndex is the clicked bag slot
	 *  (-1 = INDEX_NONE = row-name fallback, same as the other UI-click
	 *  operations). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Wallet")
	void SellItem(FName ItemRowName, int32 Count, int32 SlotIndex = -1) { ServerSellItem(ItemRowName, Count, SlotIndex); }

	/** Fired whenever the gold changes - server-side writes and the client's
	 *  OnRep of the replicated value both land here. Delta is the applied change
	 *  (0 when a clamped write changed nothing). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Wallet")
	FSnGoldChangedDelegate OnGoldChanged;

	// -------------------------------------------------------------------------
	// Hotbar. Server-authoritative (every write routes through a Server RPC
	// and replicates back). A slot holds an item's ROW NAME - a reference; the
	// item itself stays in the bag, and emptying the bag later leaves the slot
	// bound but inert (the UI shows it empty). The held visual and the widget
	// derive from the selection locally on each side.
	//
	// The selection also opens the HELD WINDOW: the selected item's UseAbility
	// is granted on the server while that item is the one in hand, and a hotbar
	// use activates THAT spec instead of granting a throwaway one
	// (RefreshHotbarGrant / ServerUseItem). A use from anywhere else - the bag,
	// an item action - keeps the engine one-shot grant, which has no window to
	// hang on.
	// -------------------------------------------------------------------------

	/** Fixed hotbar length (numeric keys 1..8 map to the slots in the PC). */
	static constexpr int32 HotbarSize = 8;

	/** Binds the slot to the item's row name (NAME_None clears the slot).
	 *  Rejects row names with no item table entry (config errors surface at
	 *  bind time, not at use time). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Hotbar")
	void SetHotbarSlot(int32 SlotIndex, FName ItemRowName) { ServerSetHotbarSlot(SlotIndex, ItemRowName); }

	/** Selects the slot (INDEX_NONE clears the selection). Replicated: every
	 *  side derives the same held visual from the selection. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Hotbar")
	void SetSelectedHotbarIndex(int32 NewIndex) { ServerSetSelectedHotbarIndex(NewIndex); }

	/** Steps the selection around the ring: Direction +1/-1 (the mouse wheel).
	 *  An empty selection picks slot 0; the ring wraps at both ends. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Hotbar")
	void CycleHotbar(int32 Direction) { ServerCycleHotbar(Direction); }

	/** Uses the SELECTED hotbar item (right-click): resolves the slot's row,
	 *  then funnels into UseItem - the local cooldown gate + UI mirror run
	 *  there for every use source, and the authoritative RPC re-checks the
	 *  server's own per-item cooldown, the bag and applies/consumes. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Hotbar")
	void UseSelectedHotbarItem();

	/** Item row bound to the slot; NAME_None when the slot is empty or the
	 *  index is out of range. */
	UFUNCTION(BlueprintPure, Category = "Sn|Hotbar")
	FName GetHotbarSlot(int32 SlotIndex) const;

	/** Currently selected slot; INDEX_NONE when nothing is selected. */
	UFUNCTION(BlueprintPure, Category = "Sn|Hotbar")
	int32 GetSelectedHotbarIndex() const { return SelectedHotbarIndex; }

	/** Row name of the selected slot; NAME_None when nothing is selected or
	 *  the slot is empty. Feeds the held-item visual (SnCharacter). */
	UFUNCTION(BlueprintPure, Category = "Sn|Hotbar")
	FName GetSelectedHotbarRowName() const;

	/** Remaining seconds of the item's use cooldown on THIS machine (0 =
	 *  ready). The client mirrors the server's cooldown math for the UI; the
	 *  server's per-item gate (UseCooldownHandles) stays authoritative. */
	UFUNCTION(BlueprintPure, Category = "Sn|Hotbar")
	float GetLocalUseCooldownRemaining(FName ItemRowName) const;

	/** Remaining fraction 0..1 of the item's use cooldown on THIS machine
	 *  (1 = just started, 0 = ready / not on cooldown) - remaining over the
	 *  total stored at start, so a widget first reading a cooldown midway
	 *  (HUD rebuild after travel) still gets an exact fraction. Feeds cooldown
	 *  ring materials: M_RingProgress sweeps to full at ready, so write
	 *  1 - Fraction for the attack-ring direction. */
	UFUNCTION(BlueprintPure, Category = "Sn|Hotbar")
	float GetLocalUseCooldownFraction(FName ItemRowName) const;

	/** Replaces the hotbar slots from a loaded save (padded/truncated to
	 *  HotbarSize; unknown row names are kept - the table may have changed).
	 *  Selection is a session choice and is NOT part of the save. */
	void CopyHotbarFrom(const TArray<FName>& NewSlots);

	/** Fired whenever the hotbar content OR the selection changes (also on
	 *  replication to clients). Widgets and the character re-derive their
	 *  state from the getters. */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Hotbar")
	FSnInventoryChangedDelegate OnHotbarChanged;

	/** Replaces both arrays wholesale from a loaded save (slot-exact - the
	 *  snapshot was captured from this component, so the arrays are already
	 *  capacity-consistent). Grid records are pure logical (no runtime handles
	 *  - those died with the old ASC); the caller then runs ReapplyGridEffects
	 *  to rebuild GE + GA. Out-of-range grid cells (grid resized between
	 *  saves) are dropped with a warning. Broadcasts the change events so the
	 *  UI refreshes. No RPC: save/load happens on the locally possessed
	 *  player.
	 *  NewGold replaces the whole wallet in the same wholesale stroke (the
	 *  snapshot IS the ledger). No default value on purpose: a 2-arg call
	 *  would silently wipe the purse, and every caller should have to say
	 *  what gold it means. */
	void CopyStateFrom(const TArray<FSnItemStack>& NewInventory, const TArray<FSnGridItem>& NewGrid, int64 NewGold);

	/** Re-applies every grid item's GE + ability set after a level travel or
	 *  continue-game restore. The ASC is fresh there (the old world's GEs
	 *  died with it). Snapshot stores BASE attributes; the equip bonuses live
	 *  in the GEs, so this replay restores them. */
	void ReapplyGridEffects();

	// --- Queries (UI) ---

	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	int32 GetItemCount(FName ItemRowName) const;

	/** Row name of the item in the given bag slot; NAME_None when the slot is
	 *  empty or the index is out of bounds. UI grid clicks can resolve the
	 *  clicked slot without touching the replicated array directly. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	FName GetItemRowNameAt(int32 SlotIndex) const;

	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	bool HasItem(FName ItemRowName, int32 Count = 1) const;

	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	const TArray<FSnItemStack>& GetInventory() const { return Inventory; }

	/** The equipment grid: sparse, one entry per occupied cell (cell number =
	 *  Row * GridSize + Column). UI renders 6x6 cells and reads GetGridItemAt. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	const TArray<FSnGridItem>& GetEquipmentGrid() const { return EquipmentGrid; }

	/** Side length of the square equipment grid (6 = 6x6, 36 cells). */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	static int32 GetGridSize() { return GridSize; }

	/** Grid item at the cell, or an invalid FSnGridItem (ItemRowName None)
	 *  when the cell is empty or out of range. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	FSnGridItem GetGridItemAt(int32 GridIndex) const;

	/** Derived level bonus of the cell: the sum of every covering rune shape
	 *  (position + rotation based - a pure function of the layout, no stored
	 *  state). Stacks: several runes boosting the same cell add up. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	int32 GetGridCellLevel(int32 GridIndex) const;

	/** Effective level of the grid item at the cell: row level + cell level
	 *  (clamped to >= 1). This is what feeds the equip GE values and the GA
	 *  growth - the number RecomputeGridEffects applies with. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	int32 GetEffectiveItemLevel(int32 GridIndex) const;

	/** Whether the cell holds an item. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	bool IsGridCellOccupied(int32 GridIndex) const;

	/** How many equipped grid items carry BondTag (bag items don't count -
	 *  only equipped items are active). UI progress displays (2/4 pieces)
	 *  read this; the recompute uses the same counting. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	int32 GetBondCount(FGameplayTag BondTag) const;

	/** All bond tags currently present on the equipped grid - one entry per
	 *  bond with at least one equipped item carrying it, sorted by tag name
	 *  (deterministic UI order). Bonds below their first tier's count are
	 *  included: the UI shows them greyed out / as progress toward tier 1. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	void GetActiveBonds(TArray<FGameplayTag>& OutBondTags) const;

	/** The bond's current LEVEL: the equipped piece count (1 item = level 1,
	 *  plain progress read, same as GetBondCount). Max level comes from
	 *  USnItemLibrary::GetBondMaxLevel (the tiers' max Count); the UI shows
	 *  "level x/y". Tier activation is derived from this count, not shown
	 *  as the level. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	int32 GetBondLevel(FGameplayTag BondTag) const;

	/** Current slot count: the Capacity attribute (raised by expansion items
	 *  via GE modifiers), defaulting to 30 while the ASC is unreachable. The
	 *  attribute is authoritative - the slot array may temporarily exceed it
	 *  (failed shrink) and ResizeToCapacity compacts/truncates as items leave. */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	int32 GetInventoryCapacity() const;

	/** True when every slot holds an item (no empty slot to drop into). */
	UFUNCTION(BlueprintPure, Category = "Sn|Inventory")
	bool IsInventoryFull() const;

	/** Fired whenever the bag changes (also on replication to clients). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Inventory")
	FSnInventoryChangedDelegate OnInventoryChanged;

	/** Fired whenever the equipment grid changes (also on replication). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Inventory")
	FSnInventoryChangedDelegate OnEquipmentChanged;

	/** Fired when the bag capacity changes (expansion items, replication). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Inventory")
	FSnCapacityChangedDelegate OnCapacityChanged;

protected:
	virtual void BeginPlay() override;

	UFUNCTION(Server, Reliable)
	void ServerPlaceInGrid(FName ItemRowName, int32 BagSlotIndex, int32 GridIndex, int32 RotationSteps);

	UFUNCTION(Server, Reliable)
	void ServerRemoveFromGrid(int32 GridIndex, int32 TargetBagSlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerMoveInGrid(int32 FromGridIndex, int32 ToGridIndex);

	UFUNCTION(Server, Reliable)
	void ServerRotateGridItem(int32 GridIndex, int32 RotationSteps);

	UFUNCTION(Server, Reliable)
	void ServerUseItem(FName ItemRowName, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerSetHotbarSlot(int32 SlotIndex, FName ItemRowName);

	UFUNCTION(Server, Reliable)
	void ServerSetSelectedHotbarIndex(int32 NewIndex);

	UFUNCTION(Server, Reliable)
	void ServerCycleHotbar(int32 Direction);

	UFUNCTION(Server, Reliable)
	void ServerDropItem(FName ItemRowName, int32 Count, int32 SlotIndex);

	UFUNCTION(Server, Reliable)
	void ServerSwapStacks(int32 FromIndex, int32 ToIndex);

	UFUNCTION(Server, Reliable)
	void ServerSortInventory();

	UFUNCTION(Server, Reliable)
	void ServerCraftItem(FName RecipeRowName, int32 Count);

	UFUNCTION(Server, Reliable)
	void ServerSellItem(FName ItemRowName, int32 Count, int32 SlotIndex);

	/** Server-side half of AddItem: clients with no authority route their
	 *  AddItem calls here (pickup actions, storage transfers) so the bag stays
	 *  server-authoritative - the local replicated arrays are never written on
	 *  the client. */
	UFUNCTION(Server, Reliable)
	void ServerAddItem(FName ItemRowName, int32 Count);

	UFUNCTION()
	void OnRep_Inventory();

	UFUNCTION()
	void OnRep_EquipmentGrid();

	UFUNCTION()
	void OnRep_HotbarSlots();

	UFUNCTION()
	void OnRep_SelectedHotbarIndex();

	UFUNCTION()
	void OnRep_Gold(int64 OldGold);

private:
	/** Rebuilds EVERY grid item's equip GE + granted ability set from scratch
	 *  (clears the whole GridRuntime map, then applies each item at its
	 *  EFFECTIVE level, ascending cell order). Used only when the ASC is
	 *  fresh - level travel / continue-game restore (ReapplyGridEffects) -
	 *  where there is nothing to keep. Runtime mutations use the incremental
	 *  ReplayItems instead (a full rebuild would tear down and re-summon
	 *  every passive ability on every grid change). Server only (client
	 *  effects replicate through the attributes). */
	void RecomputeGridEffects();

	/** Incremental recompute: re-applies the items in AffectedCells that
	 *  ACTUALLY changed - a cell whose row name and effective level both
	 *  match its applied state (GridRuntime) is skipped, because identical
	 *  inputs produce identical effects by construction (minimal impact set:
	 *  the coverage closure may suggest half the grid, the diff filters it
	 *  down to the handful that changed). Clears previous state first,
	 *  ascending cell order for a deterministic run; every other cell's
	 *  handles stay untouched, so unrelated passive abilities (summons) are
	 *  never torn down. Affected cells must come from GetAffectedGridCells
	 *  (the coverage closure) - anything outside it has an unchanged
	 *  effective level. Empty cells just drop their stale state. Recomputes
	 *  the global bond effects at the end (any layout change can move a bond
	 *  count; see RecomputeBonds). */
	void ReplayItems(const TArray<int32>& AffectedCells);

	/** FULL rebuild of the bond effects + tier abilities (no incremental path
	 *  - bonds are a global aggregate of the whole grid, any layout change can
	 *  shift any count; the tag set is small, so a full rebuild is negligible):
	 *  counts the equipped items' BondTags, activates each bond's highest
	 *  satisfied tier through the generic equip GE (SetByCaller, same
	 *  mechanism as equip modifiers) and grants the tier's EquipAbility
	 *  (grant + auto-activate through the item-equip flow, so the tier window
	 *  IS the passive's window), clears stale handles first. Server only
	 *  (client attributes replicate). */
	void RecomputeBonds();

	/** The coverage closure of a grid mutation: the changed cells plus every
	 *  rune item plus every cell a rune covers. Level changes propagate only
	 *  along rune coverage, so this superset provably contains every item
	 *  whose effective level could change (a rune's own level can be changed
	 *  by the runes covering IT, and its own coverage changes the cells it
	 *  boosts) - safe over-approximation. It is the CANDIDATE set: ReplayItems
	 *  diff-filters it (applied row/level) into the true changed set, so even
	 *  a rune-dense grid only rebuilds what actually changed.
	 *  ADDS to OutCells (never clears it): ApplyGridMutation collects the
	 *  closures of both sides of a mutation into one set. */
	void GetAffectedGridCells(const TArray<int32>& ChangedCells, TArray<int32>& OutCells) const;

	/** The ONLY way to run a grid layout mutation. Collects the coverage
	 *  closure BEFORE and AFTER Mutation and replays the union.
	 *  Why both sides are needed: the closure is derived from the CURRENT
	 *  layout, so it can only see a rune where it stands right now. A rune that
	 *  leaves the grid (or moves, or rotates) is invisible to the post-mutation
	 *  pass, and the cells it USED to cover would keep their boosted level in
	 *  GridRuntime forever - the equip GE never gets re-applied, so the
	 *  attributes stay at the boosted values ("I took the rune off and nothing
	 *  came down"). The pre-mutation pass is what catches them.
	 *  ReplayItems diff-filters the (already small) union, so the extra cells
	 *  cost nothing when nothing actually changed. */
	void ApplyGridMutation(const TArray<int32>& ChangedCells, TFunctionRef<void()> Mutation);

	/** Removes one cell's applied GE + granted abilities from the ASC (no-op
	 *  for stale handles). */
	static void ClearRuntimeState(UAbilitySystemComponent* ASC, FSnGridRuntimeState& State);

	/** Applies one item's equip GE (ability set's first GrantedEffect, else
	 *  the generic equip GE from Sn Settings) fed with the row's modifiers at
	 *  the effective level, and grants the ability set at that level; records
	 *  the handles plus the applied row name and level under the cell in
	 *  GridRuntime (the incremental replay's diff key). Returns false when no
	 *  ASC or no GE could be applied (the caller keeps the grid item and
	 *  logs). */
	bool ApplyItemToASC(UAbilitySystemComponent* ASC, const FSnItemRow* Row, FName ItemRowName, int32 EffectiveLevel, int32 GridIndex);

	/** Capacity gear guard for removals: rejects when removing the item would
	 *  leave the bag over its post-removal capacity (the equip effect shrank
	 *  the Capacity attribute). Only capacity gear (positive effective
	 *  Capacity modifier) triggers it. */
	bool WouldExceedCapacityAfterRemoval(const FSnGridItem& Item) const;

	/** Array index of the item at the cell, or INDEX_NONE. */
	int32 FindGridItemIndex(int32 GridIndex) const;

	/** Spawns an ASnItemPickup in front of the owning pawn. Returns false when
	 *  no world/pawn is available or the spawn failed - the caller rolls the
	 *  drop back instead of letting the items vanish. */
	bool SpawnDropPickup(FName ItemRowName, int32 Count);

	UAbilitySystemComponent* GetAbilitySystemComponent() const;

	/** Binds the Capacity attribute change delegate (idempotent; the ASC may
	 *  not be reachable before BeginPlay settles). */
	void BindCapacityCallbacks();

	void HandleCapacityChanged(const FOnAttributeChangeData& Data);

	/** Keeps the slot array length in sync with the capacity: pads empty slots
	 *  when growing, compacts + truncates when shrinking (fails without data
	 *  loss when items exceed the new capacity). Idempotent and cheap. */
	void ResizeToCapacity();

	/** Fallback slot count while the ASC is unreachable (matches the attribute's
	 *  initial value in USnAttributeSet). */
	static constexpr int32 DefaultCapacity = 30;

	/** Side length of the square equipment grid. Not part of the save format
	 *  (grid cells are plain numbers) - resizing only needs this constant, a
	 *  save-filter and the UI panel. */
	static constexpr int32 GridSize = 6;

	/** Hard ceiling for the gold ledger: int64 has room for far more, this keeps
	 *  the UI and any future price arithmetic sane. */
	static constexpr int64 MaxGold = 999999999999LL;

	bool bCapacityBound = false;

	/** Server-local bookkeeping (never replicated, never saved): per-cell
	 *  equip effect + granted ability handles, keyed by cell number.
	 *  Maintained by ReplayItems / RecomputeGridEffects. */
	UPROPERTY()
	TMap<int32, FSnGridRuntimeState> GridRuntime;

	/** Server-local bookkeeping (never replicated): one GE handle per ACTIVE
	 *  bond, rebuilt from the whole grid by RecomputeBonds. */
	UPROPERTY()
	TMap<FGameplayTag, FActiveGameplayEffectHandle> BondEffects;

	/** Server-local bookkeeping (never replicated): one granted ability handle
	 *  per active bond tier (FSnBondTier::EquipAbility, e.g. the Fire bond's
	 *  ignite passive), kept in lockstep with BondEffects by RecomputeBonds -
	 *  cleared (ClearAbility revokes the spec, which cancels the bActivateOnEquip
	 *  passive) whenever the tier drops. */
	UPROPERTY()
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> BondAbilities;

	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	TArray<FSnItemStack> Inventory;

	UPROPERTY(ReplicatedUsing = OnRep_EquipmentGrid)
	TArray<FSnGridItem> EquipmentGrid;

	/** Hotbar content: one item row name per slot, always HotbarSize entries
	 *  (NAME_None = empty). Replicated; initialized in the constructor. */
	UPROPERTY(ReplicatedUsing = OnRep_HotbarSlots)
	TArray<FName> HotbarSlots;

	/** Selected hotbar slot (INDEX_NONE = nothing selected). Replicated. */
	UPROPERTY(ReplicatedUsing = OnRep_SelectedHotbarIndex)
	int32 SelectedHotbarIndex = INDEX_NONE;

	/** The player's gold. Replicated; server-authoritative - AddGold and
	 *  SellItem are the only writers, a client's copy arrives by replication. */
	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	int64 Gold = 0;

	/** Server-local per-item use cooldown bookkeeping: the active cooldown
	 *  effect's handle per item row (never replicated, never saved). Handle
	 *  LIVENESS is the cooldown state (GetActiveGameplayEffect returns null
	 *  once the Duration effect expired) - per-item independent cooldowns
	 *  without dynamic gameplay tags. Stale keys are bounded by the item row
	 *  count and lazily replaced on the next use. */
	UPROPERTY()
	TMap<FName, FActiveGameplayEffectHandle> UseCooldownHandles;

	/** Server-local bookkeeping for the hotbar's HELD WINDOW (never replicated,
	 *  never saved): the ability granted for the selected slot's item, plus the
	 *  row it was granted for (the diff key). Paired with - and only with - the
	 *  selection: RefreshHotbarGrant both grants and revokes, so a spec can never
	 *  outlive the slot it belongs to (the failure mode the engine one-shot API
	 *  used to hide: a leaked spec carrying Ability.Attack, fired by every basic
	 *  attack - DEVELOPMENT_LOG 1.3). The spec itself replicates to the owning
	 *  client (ActivatableAbilities is owner-conditioned), which is how the
	 *  client-side build ghost sees the held ability; the handle does not need
	 *  to - nothing outside this component activates through it. */
	UPROPERTY()
	FGameplayAbilitySpecHandle HotbarAbilityHandle;

	UPROPERTY()
	FName HotbarAbilityRowName;

	/** Client-local use-cooldown mirror per item row (end time + total) - a
	 *  mirror of the server's gate that drives the UI readout
	 *  (GetLocalUseCooldownRemaining / GetLocalUseCooldownFraction). Never
	 *  authoritative: the server re-checks UseCooldownHandles. */
	UPROPERTY()
	TMap<FName, FSnLocalItemCooldown> LocalUseCooldowns;

	/** Re-grants the held-window ability (server only): the SELECTED slot's
	 *  UseAbility, granted while its item is the one held in hand and revoked
	 *  when the selection (or the slot's content) moves on. The granted spec is
	 *  what a hotbar use activates (ServerUseItem), and it is what the
	 *  client-side build ghost reads - "the player is holding a build item" is
	 *  this spec existing, not a class test on a table row.
	 *  Idempotent: the row name is the diff key, so re-running on an unchanged
	 *  selection does nothing. Call it AFTER the state it derives from is
	 *  written and BEFORE the change is broadcast. */
	void RefreshHotbarGrant();

	/** Server gate: whether the item's use cooldown effect is still active. */
	bool IsItemOnCooldown(FName ItemRowName) const;

	/** Applies the row's UseCD through the shared cooldown GE from Sn Settings
	 *  (Duration magnitude = SetByCaller UseCD), scaled by the player's
	 *  CooldownReduction: EffectiveCD = UseCD x (1 - CDR/100), floored at
	 *  0.1s. Records the active handle under the item row. */
	void StartItemUseCooldown(FName ItemRowName, const FSnItemRow* Row, UAbilitySystemComponent* ASC);

	/** The player's CooldownReduction attribute (point value, 0..100). */
	float GetCooldownReduction() const;
};
