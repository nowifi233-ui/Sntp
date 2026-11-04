// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/USnInventoryComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h" // FGameplayAbilitySpec
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Loot/ASnItemPickup.h"
#include "Inventory/USnItemLibrary.h"
#include "Loot/USnPickupSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Game/SnSettings.h"

USnInventoryComponent::USnInventoryComponent()
{
	SetIsReplicatedByDefault(true);

	// Fixed-length hotbar: every slot exists from the start (NAME_None =
	// empty), so the replicated array never needs resizing or padding.
	HotbarSlots.Init(NAME_None, HotbarSize);
}

void USnInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
	BindCapacityCallbacks();
	ResizeToCapacity();
}

void USnInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(USnInventoryComponent, Inventory);
	DOREPLIFETIME(USnInventoryComponent, EquipmentGrid);
	DOREPLIFETIME(USnInventoryComponent, HotbarSlots);
	DOREPLIFETIME(USnInventoryComponent, SelectedHotbarIndex);
	DOREPLIFETIME(USnInventoryComponent, Gold);
}

// --- Server-executed core operations ---

int32 USnInventoryComponent::AddItem(FName ItemRowName, int32 Count)
{
	if (!GetOwner()->HasAuthority())
	{
		// Client-side callers (pickup action blueprints, storage transfers)
		// route through the server: the local arrays are replicated copies and
		// writing them here would create ghost items the server never sees -
		// every later Server* RPC would reject them. The server's result
		// replicates back (OnRep_Inventory refreshes the UI).
		ServerAddItem(ItemRowName, Count);
		return 0;
	}

	ResizeToCapacity();
	const int32 AddedTotal = USnItemLibrary::AddItemToStacks(Inventory, ItemRowName, Count);
	if (AddedTotal > 0)
	{
		OnInventoryChanged.Broadcast();
	}
	return AddedTotal;
}

void USnInventoryComponent::ServerAddItem_Implementation(FName ItemRowName, int32 Count)
{
	if (Count <= 0 || ItemRowName.IsNone())
	{
		return;
	}
	// Server branch of AddItem: direct execute (no authority re-check).
	AddItem(ItemRowName, Count);
}

bool USnInventoryComponent::RemoveItem(FName ItemRowName, int32 Count)
{
	const bool bRemoved = USnItemLibrary::RemoveItemFromStacks(Inventory, ItemRowName, Count);
	if (bRemoved)
	{
		// Shrink check: compacts + truncates when the bag dropped below the
		// capacity again (e.g. capacity gear removed while over the limit).
		ResizeToCapacity();
		OnInventoryChanged.Broadcast();
	}
	return bRemoved;
}

bool USnInventoryComponent::RemoveItemAt(FName ItemRowName, int32 SlotIndex, int32 Count)
{
	// Slot-exact removal when the clicked slot still holds the item. The slot
	// may have changed between the client click and the server RPC (multiplayer
	// races, drags) - in that case fall back to the row-wide path instead of
	// dropping the operation.
	if (SlotIndex != INDEX_NONE && Inventory.IsValidIndex(SlotIndex)
		&& Inventory[SlotIndex].ItemRowName == ItemRowName)
	{
		if (USnItemLibrary::RemoveItemFromStackAt(Inventory, SlotIndex, Count))
		{
			ResizeToCapacity();
			OnInventoryChanged.Broadcast();
			return true;
		}
	}
	return RemoveItem(ItemRowName, Count);
}

// --- Wallet (gold) ---

void USnInventoryComponent::AddGold(int64 Amount)
{
	if (Amount == 0)
	{
		return;
	}

	AActor* const OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		// Same rule as the bag: a client never writes the ledger locally. Its
		// copy arrives by replication (OnRep_Gold), so a local write would be a
		// value the server never sees - and the next replication erases it.
		UE_LOG(LogTemp, Warning, TEXT("AddGold(%lld) called without authority; ignored"), Amount);
		return;
	}

	const int64 OldGold = Gold;
	Gold = FMath::Clamp<int64>(Gold + Amount, 0, MaxGold);
	if (Gold != OldGold)
	{
		OnGoldChanged.Broadcast(Gold, Gold - OldGold);
	}
}

void USnInventoryComponent::ServerSellItem_Implementation(FName ItemRowName, int32 Count, int32 SlotIndex)
{
	if (Count <= 0 || ItemRowName.IsNone())
	{
		return;
	}

	// The client picks what to sell, so the price is resolved HERE: an RPC can
	// carry anything, and only the table decides what is sellable (SellPrice 0
	// means both "not sellable" and "no price to show").
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(ItemRowName);
	const int32 UnitPrice = Row ? FMath::Max(0, Row->SellPrice) : 0;
	if (UnitPrice <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("SellItem: '%s' has no sell price; nothing sold"), *ItemRowName.ToString());
		return;
	}

	// Slot-exact removal re-checks the stack on the server (the shop list can
	// go stale while it stays open): nothing removed -> nothing paid. The order
	// matters - take the items first, because the credit cannot fail (AddGold
	// clamps), so there is no partial-failure path to refund.
	if (!RemoveItemAt(ItemRowName, SlotIndex, Count))
	{
		UE_LOG(LogTemp, Warning, TEXT("SellItem: could not remove %d x '%s' (slot %d); nothing sold"),
			Count, *ItemRowName.ToString(), SlotIndex);
		return;
	}

	const int64 Paid = static_cast<int64>(UnitPrice) * Count;
	AddGold(Paid);

	const APlayerState* OwnerState = Cast<APlayerState>(GetOwner());
	UE_LOG(LogTemp, Log, TEXT("SellItem: %s sold %d x '%s' @ %d gold -> %lld gold (credited %lld)"),
		OwnerState ? *OwnerState->GetPlayerName() : TEXT("<no player state>"),
		Count, *ItemRowName.ToString(), UnitPrice, Gold, Paid);
}

void USnInventoryComponent::CopyStateFrom(const TArray<FSnItemStack>& NewInventory, const TArray<FSnGridItem>& NewGrid, int64 NewGold)
{
	// Save/load: wholesale replacement. The snapshot was captured from this
	// component, so the arrays are already capacity-consistent - no resize.
	Inventory = NewInventory;

	// Wallet: also wholesale - the snapshot is the ledger, not a transaction
	// (a transaction would go through AddGold/SellItem). Unconditional
	// broadcast so a restored UI refreshes even when the number is unchanged.
	const int64 OldGold = Gold;
	Gold = FMath::Clamp<int64>(NewGold, 0, MaxGold);
	OnGoldChanged.Broadcast(Gold, Gold - OldGold);

	// Grid records are pure logical state (row name + cell + rotation) - no
	// runtime handles (those died with the old ASC); ReapplyGridEffects
	// rebuilds every item's GE + GA from them.
	EquipmentGrid.Reset(NewGrid.Num());
	for (const FSnGridItem& Item : NewGrid)
	{
		// The grid may have been resized between saves: cells outside the
		// current board cannot be rendered or applied - drop them instead of
		// letting invisible items silently keep their stats.
		const int32 CellCount = GridSize * GridSize;
		if (Item.GridIndex < 0 || Item.GridIndex >= CellCount)
		{
			UE_LOG(LogTemp, Warning, TEXT("CopyStateFrom: grid item '%s' at cell %d is outside the %dx%d grid; dropped on restore"),
				*Item.ItemRowName.ToString(), Item.GridIndex, GridSize, GridSize);
			continue;
		}
		EquipmentGrid.Add(Item);
	}
	OnInventoryChanged.Broadcast();
	OnEquipmentChanged.Broadcast();
}

void USnInventoryComponent::CopyHotbarFrom(const TArray<FName>& NewSlots)
{
	// Slot-exact replace (the snapshot was captured from this component):
	// padded/truncated to the fixed length. Unknown row names are kept - the
	// table may have changed between saves; the UI/held visual treat them as
	// missing items.
	HotbarSlots.Reset(HotbarSize);
	for (int32 Slot = 0; Slot < HotbarSize; ++Slot)
	{
		HotbarSlots.Add(NewSlots.IsValidIndex(Slot) ? NewSlots[Slot] : NAME_None);
	}
	OnHotbarChanged.Broadcast();
}

// --- Equipment grid (RPC implementations, server only) ---

void USnInventoryComponent::ServerPlaceInGrid_Implementation(FName ItemRowName, int32 BagSlotIndex, int32 GridIndex, int32 RotationSteps)
{
	const int32 CellCount = GridSize * GridSize;
	if (GridIndex < 0 || GridIndex >= CellCount)
	{
		return;
	}

	const FSnItemRow* Row = USnItemLibrary::FindItemRow(ItemRowName);
	if (!Row)
	{
		return;
	}
	// Only equipment and runes live on the grid (a rune is an item whose row
	// carries a RuneShapeID); consumables and materials stay in the bag.
	const bool bIsEquipment = USnItemLibrary::HasAnyMatchingTag(Row->ItemTags, SnGameplayTags::Item_Type_Equipment);
	const bool bIsRune = USnItemLibrary::HasAnyMatchingTag(Row->ItemTags, SnGameplayTags::Item_Type_Rune);
	if (!bIsEquipment && !bIsRune)
	{
		return;
	}
	if (IsGridCellOccupied(GridIndex))
	{
		return;
	}
	if (!GetAbilitySystemComponent())
	{
		// No ASC = no effect could ever apply: reject instead of swallowing
		// the item out of the bag.
		return;
	}

	// Take the item out of the bag: the clicked slot when it still holds the
	// item, row-name fallback otherwise (RemoveItemAt handles both).
	if (!RemoveItemAt(ItemRowName, BagSlotIndex, 1))
	{
		return;
	}

	FSnGridItem Item;
	Item.ItemRowName = ItemRowName;
	Item.GridIndex = GridIndex;
	Item.RotationSteps = static_cast<uint8>(((RotationSteps % 4) + 4) % 4);

	// Incremental: only the coverage closure of this cell is re-applied (the
	// new item + any rune linkage) - the other cells' passives stay up.
	ApplyGridMutation({GridIndex}, [&]() { EquipmentGrid.Add(Item); });
}

void USnInventoryComponent::ServerRemoveFromGrid_Implementation(int32 GridIndex, int32 TargetBagSlotIndex)
{
	const int32 ArrayIndex = FindGridItemIndex(GridIndex);
	if (ArrayIndex == INDEX_NONE)
	{
		return;
	}
	const FSnGridItem Item = EquipmentGrid[ArrayIndex];

	// Capacity gear guard: removing the equip effect shrinks the Capacity
	// attribute. Reject the removal when the bag holds more items than the
	// post-removal capacity - otherwise tail items would end up beyond the UI
	// grid (the grid is capacity-driven).
	if (WouldExceedCapacityAfterRemoval(Item))
	{
		return;
	}

	// Bag first: refuse when there's no room (keeps the item safe).
	if (TargetBagSlotIndex == INDEX_NONE)
	{
		if (AddItem(Item.ItemRowName, 1) != 1)
		{
			return;
		}
	}
	else
	{
		// Specified target: only an empty slot or a non-full stack of the same
		// item is accepted - anything else rejects and the item stays on the
		// grid (drag snaps back).
		ResizeToCapacity();
		if (!Inventory.IsValidIndex(TargetBagSlotIndex))
		{
			return;
		}
		FSnItemStack& Target = Inventory[TargetBagSlotIndex];
		if (Target.ItemRowName.IsNone())
		{
			Target.ItemRowName = Item.ItemRowName;
			Target.Count = 1;
		}
		else if (Target.ItemRowName == Item.ItemRowName
			&& Target.Count < USnItemLibrary::GetItemStackSize(Item.ItemRowName))
		{
			++Target.Count;
		}
		else
		{
			return;
		}
		OnInventoryChanged.Broadcast();
	}

	// The mutation runs INSIDE ApplyGridMutation: taking the rune off the grid
	// is exactly the case where the cells it boosted can only be found before
	// the removal (afterwards the closure cannot see the rune any more, and the
	// boosted items kept their level - the attributes never came back down).
	ApplyGridMutation({GridIndex}, [&]() { EquipmentGrid.RemoveAt(ArrayIndex); });
}

void USnInventoryComponent::ServerMoveInGrid_Implementation(int32 FromGridIndex, int32 ToGridIndex)
{
	const int32 CellCount = GridSize * GridSize;
	if (FromGridIndex == ToGridIndex
		|| FromGridIndex < 0 || FromGridIndex >= CellCount
		|| ToGridIndex < 0 || ToGridIndex >= CellCount)
	{
		return;
	}

	FSnGridItem* Moving = nullptr;
	FSnGridItem* Occupied = nullptr;
	for (FSnGridItem& Item : EquipmentGrid)
	{
		if (Item.GridIndex == FromGridIndex)
		{
			Moving = &Item;
		}
		else if (Item.GridIndex == ToGridIndex)
		{
			Occupied = &Item;
		}
	}
	if (!Moving)
	{
		return;
	}
	// Empty target moves; occupied target swaps the two items (same drag-drop
	// freedom as the bag's SwapStacks). Both endpoints are in the closure, and
	// a moved RUNE needs both sides too: its old coverage is only visible while
	// it still stands on the from-cell (ApplyGridMutation).
	ApplyGridMutation({FromGridIndex, ToGridIndex}, [&]()
	{
		if (Occupied)
		{
			Swap(Moving->GridIndex, Occupied->GridIndex);
		}
		else
		{
			Moving->GridIndex = ToGridIndex;
		}
	});
}

void USnInventoryComponent::ServerRotateGridItem_Implementation(int32 GridIndex, int32 RotationSteps)
{
	const int32 ArrayIndex = FindGridItemIndex(GridIndex);
	if (ArrayIndex == INDEX_NONE)
	{
		return;
	}
	// Delta rotation: the step count ADDS to the current rotation (a "rotate"
	// button clicks +1 each press); normalized to mod 4 so four presses return
	// to the authored orientation. Rotating a rune re-covers a different set of
	// cells, so the closure has to see the pre-rotation coverage as well.
	FSnGridItem& Item = EquipmentGrid[ArrayIndex];
	ApplyGridMutation({GridIndex}, [&]()
	{
		Item.RotationSteps = static_cast<uint8>(((Item.RotationSteps + RotationSteps) % 4 + 4) % 4);
	});
}

void USnInventoryComponent::RecomputeGridEffects()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Clear everything from the previous layout first - stale effects and
	// abilities would stack forever otherwise (the derived levels change on
	// every grid mutation, so nothing survives between recomputes).
	for (TPair<int32, FSnGridRuntimeState>& Entry : GridRuntime)
	{
		ClearRuntimeState(ASC, Entry.Value);
	}
	GridRuntime.Reset();

	// Deterministic apply order: ascending cell index (a stable run order for
	// effects that stack, e.g. several capacity gears).
	TArray<FSnGridItem> Sorted = EquipmentGrid;
	Sorted.Sort([](const FSnGridItem& A, const FSnGridItem& B) { return A.GridIndex < B.GridIndex; });

	for (const FSnGridItem& Item : Sorted)
	{
		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row)
		{
			UE_LOG(LogTemp, Warning, TEXT("RecomputeGridEffects: item row '%s' no longer exists; grid item skipped"),
				*Item.ItemRowName.ToString());
			continue;
		}
		if (!ApplyItemToASC(ASC, Row, Item.ItemRowName, GetEffectiveItemLevel(Item.GridIndex), Item.GridIndex))
		{
			UE_LOG(LogTemp, Warning, TEXT("RecomputeGridEffects: failed to apply '%s' at cell %d (equip GE missing in Sn Settings or ASC unreachable)"),
				*Item.ItemRowName.ToString(), Item.GridIndex);
		}
	}

	// The bond effects are part of the same derived-to-apply set: a fresh ASC
	// (level travel / continue-game) rebuilds them here alongside the items.
	RecomputeBonds();
}

void USnInventoryComponent::ReplayItems(const TArray<int32>& AffectedCells)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Ascending cell order: a stable run order for effects that stack (the
	// untouched cells keep whatever order they were applied in; only the
	// affected subset is replayed).
	TArray<int32> Cells = AffectedCells;
	Cells.Sort();

	for (const int32 Cell : Cells)
	{
		const FSnGridItem Item = GetGridItemAt(Cell);
		if (!Item.IsValid())
		{
			// Empty cell: drop whatever state the previous occupant left.
			if (FSnGridRuntimeState* State = GridRuntime.Find(Cell))
			{
				ClearRuntimeState(ASC, *State);
				GridRuntime.Remove(Cell);
			}
			continue;
		}

		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row)
		{
			UE_LOG(LogTemp, Warning, TEXT("ReplayItems: item row '%s' no longer exists; grid item skipped"),
				*Item.ItemRowName.ToString());
			continue;
		}

		// Minimal impact set: the coverage closure is a safe SUPERSET, but a
		// cell whose row name AND effective level both match its applied state
		// produces exactly the same effects (GE values and GA spec levels are
		// f(level)) - rebuilding it would tear down and re-create the same
		// passives for nothing. Skip it; only genuinely changed cells rebuild.
		// This keeps rune-dense layouts O(changed) instead of O(all runes +
		// all covered cells).
		const int32 NewLevel = GetEffectiveItemLevel(Cell);
		if (const FSnGridRuntimeState* State = GridRuntime.Find(Cell))
		{
			if (State->AppliedRowName == Item.ItemRowName && State->AppliedLevel == NewLevel)
			{
				continue;
			}
		}

		// Rebuild just this cell: clear its previous state (the handles are
		// keyed by cell, so unrelated cells' passives - summons etc. - stay up).
		if (FSnGridRuntimeState* State = GridRuntime.Find(Cell))
		{
			ClearRuntimeState(ASC, *State);
			GridRuntime.Remove(Cell);
		}
		if (!ApplyItemToASC(ASC, Row, Item.ItemRowName, NewLevel, Cell))
		{
			UE_LOG(LogTemp, Warning, TEXT("ReplayItems: failed to apply '%s' at cell %d (equip GE missing in Sn Settings or ASC unreachable)"),
				*Item.ItemRowName.ToString(), Cell);
		}
	}

	// Bonds are a global aggregate: any grid mutation can move any count, so
	// every replay ends with a full bond rebuild (cheap - a handful of tags).
	RecomputeBonds();
}

void USnInventoryComponent::RecomputeBonds()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Full rebuild (the same policy as RecomputeGridEffects): bonds are a
	// GLOBAL aggregate - every layout change can shift every count, so an
	// incremental diff would scan the same cells for no gain. The tag set is
	// small (a handful of bonds), so clearing + reapplying is negligible.
	// Both the modifier effects and the granted tier abilities are torn down:
	// ClearAbility revokes the spec, which cancels the bActivateOnEquip
	// passive (e.g. the ignite GA) and unbinds its listeners.
	for (const TPair<FGameplayTag, FActiveGameplayEffectHandle>& Entry : BondEffects)
	{
		ASC->RemoveActiveGameplayEffect(Entry.Value);
	}
	BondEffects.Reset();
	for (const TPair<FGameplayTag, FGameplayAbilitySpecHandle>& Entry : BondAbilities)
	{
		ASC->ClearAbility(Entry.Value);
	}
	BondAbilities.Reset();

	// Count equipped items per bond tag (bag items do NOT count - only
	// equipped items are active; the UI query GetBondCount mirrors this).
	TMap<FGameplayTag, int32> BondCounts;
	for (const FSnGridItem& Item : EquipmentGrid)
	{
		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row)
		{
			continue;
		}
		for (const FGameplayTag& Tag : Row->BondTags)
		{
			BondCounts.FindOrAdd(Tag) += 1;
		}
	}

	const UDataTable* BondTable = USnSettings::Get()->GetBondDataTable();
	if (!BondTable)
	{
		return; // No table = bonds not configured; effects/abilities stay cleared.
	}

	// Same generic equip GE as the equip modifiers (Infinite, SetByCaller) -
	// separate GE instances stack on top of the per-item ones, so bond and
	// equip bonuses add up independently.
	const TSubclassOf<UGameplayEffect> BondGE = USnSettings::Get()->GetGenericEquipEffectClass();

	for (const TPair<FGameplayTag, int32>& Entry : BondCounts)
	{
		// Row name IS the tag name (e.g. "Item.Bond.Fire").
		const FSnBondRow* BondRow = BondTable->FindRow<FSnBondRow>(Entry.Key.GetTagName(), TEXT("USnInventoryComponent::RecomputeBonds"));
		if (!BondRow)
		{
			continue;
		}

		// Highest satisfied tier: Tiers are ascending by Count, so the first
		// match from the back is the strongest.
		const FSnBondTier* ActiveTier = nullptr;
		for (int32 i = BondRow->Tiers.Num() - 1; i >= 0; --i)
		{
			if (Entry.Value >= BondRow->Tiers[i].Count)
			{
				ActiveTier = &BondRow->Tiers[i];
				break;
			}
		}

		if (!ActiveTier)
		{
			continue;
		}

		// Tier bonuses are two independent parts (either may be empty): the
		// modifiers ride the shared generic equip GE - bond values are used as
		// authored (no level scaling): Level 1.
		if (ActiveTier->Modifiers.Num() > 0)
		{
			const FActiveGameplayEffectHandle Handle = USnItemLibrary::ApplyItemModifiers(ASC, BondGE, ActiveTier->Modifiers, /*Level=*/1);
			if (Handle.IsValid())
			{
				BondEffects.Add(Entry.Key, Handle);
			}
		}

		// ...and the tier's passive ability (e.g. the Fire bond's ignite GA)
		// is granted + activated through the same flow as the item rows'
		// EquipAbilities (bActivateOnEquip, grant + activate in one): the tier
		// window IS the passive's window - losing the tier clears it in the
		// rebuild's clear loop above, which cancels the passive.
		if (ActiveTier->EquipAbility)
		{
			TArray<FGameplayAbilitySpecHandle> Handles;
			USnItemLibrary::GrantEquipAbilities(ASC, { ActiveTier->EquipAbility }, /*Level=*/1, Handles);
			if (Handles.Num() > 0)
			{
				BondAbilities.Add(Entry.Key, Handles[0]);
			}
		}
	}
}

int32 USnInventoryComponent::GetBondCount(FGameplayTag BondTag) const
{
	if (!BondTag.IsValid())
	{
		return 0;
	}

	int32 Count = 0;
	for (const FSnGridItem& Item : EquipmentGrid)
	{
		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row)
		{
			continue;
		}
		if (Row->BondTags.HasTagExact(BondTag))
		{
			++Count;
		}
	}
	return Count;
}

void USnInventoryComponent::GetActiveBonds(TArray<FGameplayTag>& OutBondTags) const
{
	OutBondTags.Reset();

	TSet<FGameplayTag> ActiveTags;
	for (const FSnGridItem& Item : EquipmentGrid)
	{
		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row)
		{
			continue;
		}
		for (const FGameplayTag& Tag : Row->BondTags.GetGameplayTagArray())
		{
			ActiveTags.Add(Tag); // set dedups across items
		}
	}

	// Deterministic UI order: tag name ascending.
	OutBondTags = ActiveTags.Array();
	OutBondTags.Sort([](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
}

int32 USnInventoryComponent::GetBondLevel(FGameplayTag BondTag) const
{
	// Level = the currently owned piece count (1 item = level 1), so the UI
	// shows plain progress "level x/y" where y = GetBondMaxLevel (the last
	// tier's Count). Tier activation is derived from this count elsewhere.
	return GetBondCount(BondTag);
}

void USnInventoryComponent::GetAffectedGridCells(const TArray<int32>& ChangedCells, TArray<int32>& OutCells) const
{
	// Additive, not an assignment: ApplyGridMutation calls this on both sides of
	// a mutation and accumulates into one set.
	for (const int32 ChangedCell : ChangedCells)
	{
		OutCells.AddUnique(ChangedCell);
	}

	// Coverage closure: level changes propagate only along rune coverage.
	// Every rune's effective level can change (the cells covering IT may have
	// changed), and every rune's own coverage changes the boosted cells'
	// levels - so the closure is: the changed cells + all rune cells + all
	// covered cells. A superset of the true affected set (safe, never drifts);
	// typically a handful of cells even when runes are present.
	for (const FSnGridItem& Item : EquipmentGrid)
	{
		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row || Row->RuneShapeID.IsNone())
		{
			continue;
		}

		OutCells.AddUnique(Item.GridIndex);
		TArray<int32> Covered;
		USnItemLibrary::GetCoveredCells(Item, GridSize, Covered);
		for (const int32 CoveredCell : Covered)
		{
			OutCells.AddUnique(CoveredCell);
		}
	}
}

void USnInventoryComponent::ApplyGridMutation(const TArray<int32>& ChangedCells, TFunctionRef<void()> Mutation)
{
	// BEFORE the mutation: while a rune still stands where it was, the closure
	// sees every cell it boosts. This is the only moment those cells can be
	// found - a removed rune has no coverage to report, a moved one reports its
	// NEW coverage, and a rotated one its new orientation (see the header).
	TArray<int32> Affected;
	GetAffectedGridCells(ChangedCells, Affected);

	Mutation();

	// AFTER: the new coverage (a rune that just moved here, a newly covered
	// cell, ...). Same ChangedCells - the vacated cell itself still has to drop
	// its runtime state.
	GetAffectedGridCells(ChangedCells, Affected);

	// ReplayItems diff-filters the union back down to the cells whose applied
	// row/level actually differ, then rebuilds the global bond effects.
	ReplayItems(Affected);

	// Unconditional (also when there is no ASC to replay into): the UI mirrors
	// the layout, not the effects.
	OnEquipmentChanged.Broadcast();
}

void USnInventoryComponent::ClearRuntimeState(UAbilitySystemComponent* ASC, FSnGridRuntimeState& State)
{
	if (State.EquipEffect.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(State.EquipEffect);
	}
	for (const FGameplayAbilitySpecHandle& Handle : State.Abilities)
	{
		// ClearAbility is a no-op for expired handles; active abilities end
		// through the normal ability-end path (passives clean themselves up,
		// e.g. a summoned companion).
		ASC->ClearAbility(Handle);
	}
}

bool USnInventoryComponent::ApplyItemToASC(UAbilitySystemComponent* ASC, const FSnItemRow* Row, FName ItemRowName, int32 EffectiveLevel, int32 GridIndex)
{
	// Equip effect: the global generic equip GE from Sn Settings always
	// carries the row's EquipModifiers (SetByCaller) - the old per-item
	// equip GE (the ability set's first granted effect) is gone.
	const USnSettings* Settings = USnSettings::Get();
	const TSubclassOf<UGameplayEffect> EquipGE = Settings ? Settings->GetGenericEquipEffectClass() : nullptr;

	const FActiveGameplayEffectHandle Handle = ASC
		? USnItemLibrary::ApplyItemModifiers(ASC, EquipGE, Row->EquipModifiers, EffectiveLevel)
		: FActiveGameplayEffectHandle();
	if (!Handle.IsValid())
	{
		return false;
	}

	// Record under the cell: the next replay of this cell revokes exactly
	// this state, other cells stay untouched.
	FSnGridRuntimeState& State = GridRuntime.FindOrAdd(GridIndex);
	State.EquipEffect = Handle;

	// Grant the item's equip abilities at the EFFECTIVE level (GA growth).
	// GrantEquipAbilities activates the passives right after the grant
	// (bActivateOnEquip: grant + activate in one, owned by the equip flow), so
	// placing and restore-after-travel take the same path - the inventory
	// never has to fire them from the outside.
	USnItemLibrary::GrantEquipAbilities(ASC, Row->EquipAbilities, EffectiveLevel, State.Abilities);

	// Diff key for the incremental replay (ReplayItems): this exact row +
	// level is what the current effects were built from; seeing the same
	// pair again means there is nothing to rebuild.
	State.AppliedRowName = ItemRowName;
	State.AppliedLevel = EffectiveLevel;
	return true;
}

void USnInventoryComponent::ReapplyGridEffects()
{
	// After a level travel (or continue-game) the ASC is fresh: the equip GEs
	// and granted ability sets died with the old world. Re-apply everything
	// from the grid so removal still works - without this, the bonuses stay
	// baked into the base attributes and a later recompute would stack them
	// forever (the snapshot stores BASE attributes; the bonuses live in the
	// GEs). Grid state is pure logical, so this is a plain recompute.
	RecomputeGridEffects();

	// The hotbar's held window died with that same ASC. Re-grant it from the
	// selection (a no-op when nothing is selected, which is the normal case -
	// the selection is a session choice and does not travel).
	RefreshHotbarGrant();
}

// --- RPC implementations (bag, server only) ---

void USnInventoryComponent::ServerUseItem_Implementation(FName ItemRowName, int32 SlotIndex)
{
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(ItemRowName);
	if (!Row || (Row->UseModifiers.Num() == 0 && !Row->UseAbility))
	{
		// Nothing to apply and nothing to activate: the item has no use flow
		// (was: required per-item use effect class; a use is now modifiers, an
		// ability, or both).
		return;
	}

	// Per-item cooldown gate FIRST (before the bag check): an item on
	// cooldown is rejected and NOT consumed. The gate is the handle's
	// liveness - GetActiveGameplayEffect returns null once the cooldown
	// effect expired.
	if (IsItemOnCooldown(ItemRowName))
	{
		return;
	}
	if (!HasItem(ItemRowName, 1))
	{
		return;
	}
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		// No ASC = nothing the modifiers could be applied to; return without
		// consuming, otherwise the item disappears for nothing.
		return;
	}

	if (Row->UseModifiers.Num() > 0)
	{
		// Duration items (UseDurationSeconds > 0) route through the buff GE
		// (Has Duration, length from the row); instant consumables use the use
		// GE. SetByCaller carries the values in both cases.
		const USnSettings* Settings = USnSettings::Get();
		const TSubclassOf<UGameplayEffect> UseGE = Settings && Row->UseDurationSeconds > 0.f
			? Settings->GetGenericBuffEffectClass()
			: Settings->GetGenericUseEffectClass();
		if (!UseGE)
		{
			UE_LOG(LogTemp, Warning, TEXT("ServerUseItem: no generic use/buff GE configured in Sn Settings; item not consumed"));
			return;
		}
		USnItemLibrary::ApplyItemModifiers(ASC, UseGE, Row->UseModifiers, Row->Level, Row->UseDurationSeconds);
	}

	if (Row->UseAbility)
	{
		// "Use also carries one ability", in one of two shapes:
		//
		// HELD (this row is the item in hand - the hotbar's held window): the
		// spec is already granted (RefreshHotbarGrant), so a use only activates
		// it. That spec is also what a client's build ghost reads, which is why
		// holding a kit is one fact and not two guesses.
		//
		// OTHERWISE (bag right-click, item action, a slot whose window is gone -
		// and, deliberately, a held handle whose spec is no longer in the ASC):
		// there is no window to hang a spec on, so the engine one-shot API
		// grants it for this use alone - GiveAbilityAndActivateOnce marks the
		// spec for removal the moment the instance ends and clears it itself on
		// activation failure (returns an invalid handle).
		//
		// Both shapes abort the use when the activation is refused (the
		// ability's own gates: placement, cost, cooldown): no consume, no use
		// cooldown, and the held spec survives so the player can simply try
		// again. The held shape is the looser of the two - the one-shot shape
		// cannot be retried without another use. Modifiers applied above
		// (buff-style uses) stay either way - a lossless outcome for the
		// player. Both classes are ServerOnly by contract (USnGA_Build /
		// USnGA_OneShotAttack): the instance only runs on the server, and every
		// client sees the result through replication.
		//
		// The held branch asks the ASC whether the spec is really there (not just
		// whether the handle looks valid) so that a cleared-behind-our-back spec
		// falls back to the one-shot shape and the use still happens, instead of
		// TryActivateAbility returning false and the use quietly doing nothing.
		const bool bHeld = HotbarAbilityRowName == ItemRowName && HotbarAbilityHandle.IsValid()
			&& ASC->FindAbilitySpecFromHandle(HotbarAbilityHandle) != nullptr;
		if (bHeld)
		{
			if (!ASC->TryActivateAbility(HotbarAbilityHandle))
			{
				return;
			}
		}
		else
		{
			FGameplayAbilitySpec UseSpec(Row->UseAbility, FMath::Max(1, Row->Level));
			if (!ASC->GiveAbilityAndActivateOnce(UseSpec).IsValid())
			{
				return;
			}
		}
	}

	if (Row->UseCD > 0.f)
	{
		StartItemUseCooldown(ItemRowName, Row, ASC);
	}

	if (Row->bConsumeOnUse)
	{
		// Consume from the clicked slot (row-name fallback inside).
		RemoveItemAt(ItemRowName, SlotIndex, 1);
	}
}

void USnInventoryComponent::ServerDropItem_Implementation(FName ItemRowName, int32 Count, int32 SlotIndex)
{
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(ItemRowName);
	if (!Row || !Row->bCanDrop || Count <= 0)
	{
		return;
	}
	if (!RemoveItemAt(ItemRowName, SlotIndex, Count))
	{
		return;
	}
	if (!SpawnDropPickup(ItemRowName, Count))
	{
		// No world/pawn to drop in front of: put the items back instead of
		// letting them vanish. The slot we just emptied has room, so the
		// rollback always succeeds (it cannot fill up again mid-call).
		AddItem(ItemRowName, Count);
	}
}

void USnInventoryComponent::ServerSwapStacks_Implementation(int32 FromIndex, int32 ToIndex)
{
	ResizeToCapacity();
	if (FromIndex == ToIndex || !Inventory.IsValidIndex(FromIndex) || !Inventory.IsValidIndex(ToIndex))
	{
		return;
	}

	FSnItemStack& Source = Inventory[FromIndex];
	FSnItemStack& Target = Inventory[ToIndex];
	if (!Source.ItemRowName.IsNone() && !Target.ItemRowName.IsNone()
		&& Source.ItemRowName == Target.ItemRowName)
	{
		// Merge into the target, capped at the stack size. Overflow stays in
		// the source slot, which empties out when fully merged.
		const int32 StackSize = USnItemLibrary::GetItemStackSize(Target.ItemRowName);
		const int32 Room = StackSize - Target.Count;
		const int32 Merged = FMath::Min(Source.Count, Room);
		Target.Count += Merged;
		Source.Count -= Merged;
		if (Source.Count <= 0)
		{
			Source.ItemRowName = NAME_None;
		}
	}
	else
	{
		// Fixed-slot model: swap the slot contents outright - empty slots swap
		// freely, so dropping onto an empty slot places the item there.
		Swap(Inventory[FromIndex], Inventory[ToIndex]);
	}

	OnInventoryChanged.Broadcast();
}

void USnInventoryComponent::ServerSortInventory_Implementation()
{
	ResizeToCapacity();
	USnItemLibrary::SortStacks(Inventory);
	OnInventoryChanged.Broadcast();
}

void USnInventoryComponent::ServerCraftItem_Implementation(FName RecipeRowName, int32 Count)
{
	const FSnCraftRecipeRow* Recipe = USnItemLibrary::FindRecipeRow(RecipeRowName);
	if (!Recipe || Recipe->Materials.Num() == 0 || Count <= 0
		|| !USnItemLibrary::FindItemRow(Recipe->OutputItemRowName))
	{
		return;
	}

	// Materials first, all-or-nothing: no partial consumption.
	for (const FSnRecipeIngredient& Ingredient : Recipe->Materials)
	{
		if (Ingredient.Count <= 0 || GetItemCount(Ingredient.ItemRowName) < Ingredient.Count * Count)
		{
			return;
		}
	}

	// Output fit check BEFORE consuming anything (material-loss prevention):
	// simulate the add on a copy of the bag. The bag is a small fixed array,
	// so the copy costs nothing at craft frequency.
	TArray<FSnItemStack> Simulated = Inventory;
	if (USnItemLibrary::AddItemToStacks(Simulated, Recipe->OutputItemRowName, Recipe->OutputCount * Count)
		< Recipe->OutputCount * Count)
	{
		return; // Bag full: nothing changes.
	}

	// Consume then add. The simulation guarantees the add succeeds, so no
	// rollback path is needed.
	for (const FSnRecipeIngredient& Ingredient : Recipe->Materials)
	{
		USnItemLibrary::RemoveItemFromStacks(Inventory, Ingredient.ItemRowName, Ingredient.Count * Count);
	}
	USnItemLibrary::AddItemToStacks(Inventory, Recipe->OutputItemRowName, Recipe->OutputCount * Count);
	ResizeToCapacity();
	OnInventoryChanged.Broadcast();
}

// --- Hotbar (server-only state + per-item use cooldowns) ---

void USnInventoryComponent::RefreshHotbarGrant()
{
	// Server-only state: ClearAbility / GiveAbility both require authority and
	// log an error without it. A client never writes this - it sees the result
	// through the replicated spec, which is all it needs (the build ghost).
	if (!GetOwner()->HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();

	const FName RowName = GetSelectedHotbarRowName();

	// Unchanged selection, live grant - the row name is the diff key, so the
	// steady state costs one lookup. "Live" is asked of the CURRENT ASC instead
	// of read off the handle: a handle is globally unique and never reused
	// (GameplayAbilitySpecHandle.h), but it is still just a number kept here -
	// anything that clears the spec behind our back (or an ASC rebuilt under a
	// surviving component) leaves it valid-looking while the spec is long gone,
	// and this early-out would then keep the window shut for good. The lookup
	// answers what is true right now instead.
	if (HotbarAbilityRowName == RowName && ASC
		&& ASC->FindAbilitySpecFromHandle(HotbarAbilityHandle) != nullptr)
	{
		return;
	}

	// Revoke by the stored handle whatever comes next, so grant and revoke stay
	// paired: a spec granted here can never outlive the slot it belongs to. (A
	// leaked spec is not a cosmetic problem in this project - one carrying
	// Ability.Attack is fired by every basic attack; that is what the engine's
	// one-shot API used to paper over - DEVELOPMENT_LOG 1.3.)
	if (ASC && HotbarAbilityHandle.IsValid())
	{
		ASC->ClearAbility(HotbarAbilityHandle);
	}
	HotbarAbilityHandle = FGameplayAbilitySpecHandle();
	HotbarAbilityRowName = RowName;

	const FSnItemRow* Row = USnItemLibrary::FindItemRow(RowName);
	if (!ASC || !Row || !Row->UseAbility)
	{
		return; // Empty slot, or an item whose use carries no ability: window shut.
	}

	// Granted at exactly the level the one-shot path uses, so the two entries
	// into a use are interchangeable. SourceObject names the component, the
	// same convention the companions use - it says where a spec came from when
	// you are looking at one in a debugger.
	FGameplayAbilitySpec Spec(Row->UseAbility, FMath::Max(1, Row->Level), INDEX_NONE, this);
	HotbarAbilityHandle = ASC->GiveAbility(Spec);
}

void USnInventoryComponent::ServerSetHotbarSlot_Implementation(int32 SlotIndex, FName ItemRowName)
{
	if (!HotbarSlots.IsValidIndex(SlotIndex))
	{
		return;
	}
	// Only existing item rows may be bound (NAME_None clears the slot):
	// config errors surface at bind time, not at use time.
	if (!ItemRowName.IsNone() && !USnItemLibrary::FindItemRow(ItemRowName))
	{
		return;
	}
	if (HotbarSlots[SlotIndex] == ItemRowName)
	{
		return; // No change - no broadcast (the UI re-renders on change only).
	}
	HotbarSlots[SlotIndex] = ItemRowName;
	// Re-binding the SELECTED slot swaps what is held: the held window has to
	// follow, exactly as it does when the selection itself moves.
	RefreshHotbarGrant();
	OnHotbarChanged.Broadcast();
}

void USnInventoryComponent::ServerSetSelectedHotbarIndex_Implementation(int32 NewIndex)
{
	// INDEX_NONE clears the selection; anything else must be a real slot.
	if (NewIndex < INDEX_NONE || NewIndex >= HotbarSize)
	{
		return;
	}
	if (SelectedHotbarIndex == NewIndex)
	{
		return;
	}
	SelectedHotbarIndex = NewIndex;
	// Held window first, broadcast second: the grant is part of the state the
	// event announces, so a listener never sees a selection whose ability has
	// not been granted yet.
	RefreshHotbarGrant();
	OnHotbarChanged.Broadcast();
}

void USnInventoryComponent::ServerCycleHotbar_Implementation(int32 Direction)
{
	// Ring around the fixed slots (the mouse wheel). An empty selection picks
	// slot 0; the ring wraps at both ends (negative directions included).
	if (HotbarSlots.Num() == 0)
	{
		return;
	}
	const int32 Start = SelectedHotbarIndex < 0 ? 0 : SelectedHotbarIndex;
	const int32 Next = ((Start + Direction) % HotbarSize + HotbarSize) % HotbarSize;
	if (Next == SelectedHotbarIndex)
	{
		return;
	}
	SelectedHotbarIndex = Next;
	RefreshHotbarGrant();
	OnHotbarChanged.Broadcast();
}

void USnInventoryComponent::UseItem(FName ItemRowName, int32 SlotIndex)
{
	// Shared client-side entry for EVERY use source (selected hotbar item,
	// bag right-click, ...): local cooldown gate + UI mirror first, then the
	// authoritative RPC. The server re-checks its own per-item gate (handle
	// liveness) and the bag inside the RPC, so a stale local clock can only
	// cause a silent server rejection - never an early second use, and never
	// a consume of an item that was on cooldown.
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(ItemRowName);
	if (!Row)
	{
		return;
	}
	if (GetLocalUseCooldownRemaining(ItemRowName) > 0.f)
	{
		return;
	}
	if (Row->UseCD > 0.f)
	{
		// Same formula as the server (StartItemUseCooldown), from the local
		// replica of the CooldownReduction attribute - close enough for a
		// visual readout. Total is stored alongside the end time so the
		// fraction getter stays exact for readers that arrive mid-cooldown
		// (HUD rebuilt after a travel). Primes even when the RPC later fails
		// server-side (empty bag): the mirror may only delay a use, never
		// allow an early one - the server gate decides.
		const float EffectiveCD = FMath::Max(0.1f, Row->UseCD * (1.f - GetCooldownReduction() / 100.f));
		FSnLocalItemCooldown Local;
		Local.EndTime = GetWorld()->GetTimeSeconds() + EffectiveCD;
		Local.TotalDuration = EffectiveCD;
		LocalUseCooldowns.Add(ItemRowName, Local);
	}

	// The authoritative half (bag check, server cooldown gate, use effects,
	// consume) runs on the server through the use RPC.
	ServerUseItem(ItemRowName, SlotIndex);
}

void USnInventoryComponent::UseSelectedHotbarItem()
{
	const FName RowName = GetSelectedHotbarRowName();
	if (RowName.IsNone())
	{
		return;
	}

	// Gate + mirror live in UseItem - the shared entry for every use source.
	// Row-name semantics (INDEX_NONE): the server consumes by row, not from a
	// specific bag slot.
	UseItem(RowName, INDEX_NONE);
}

bool USnInventoryComponent::IsItemOnCooldown(FName ItemRowName) const
{
	const FActiveGameplayEffectHandle* Handle = UseCooldownHandles.Find(ItemRowName);
	if (!Handle || !Handle->IsValid())
	{
		return false;
	}
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// Handle liveness IS the cooldown state: GetActiveGameplayEffect
		// returns null once the duration effect expired. Per-item independent
		// cooldowns without dynamic gameplay tags.
		return ASC->GetActiveGameplayEffect(*Handle) != nullptr;
	}
	return false;
}

void USnInventoryComponent::StartItemUseCooldown(FName ItemRowName, const FSnItemRow* Row, UAbilitySystemComponent* ASC)
{
	// Cooldown reduction (point value 0..100, 10 = 10%): the reduction only
	// affects cooldowns STARTED while the stat is in effect - a running
	// cooldown keeps the length it started with. Floored at 0.1s so a use
	// never degrades into a double-fire.
	float EffectiveCD = Row->UseCD * (1.f - GetCooldownReduction() / 100.f);
	EffectiveCD = FMath::Max(0.1f, EffectiveCD);

	const USnSettings* Settings = USnSettings::Get();
	const TSubclassOf<UGameplayEffect> CDGE = Settings ? Settings->GetUseCooldownEffectClass() : nullptr;
	if (!CDGE)
	{
		UE_LOG(LogTemp, Warning, TEXT("StartItemUseCooldown: no use-cooldown GE configured in Sn Settings; item '%s' used without a cooldown"),
			*ItemRowName.ToString());
		return;
	}

	// One shared cooldown asset for every item (Duration policy): the row's
	// length rides as a SetByCaller duration magnitude - see GE_UseCooldown.
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(CDGE, FMath::Max(1, Row->Level), Context);
	if (!Spec.IsValid())
	{
		return;
	}
	Spec.Data->SetSetByCallerMagnitude(SnGameplayTags::SetByCaller_UseCD, EffectiveCD);
	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
	if (Handle.IsValid())
	{
		UseCooldownHandles.Add(ItemRowName, Handle);
	}
}

float USnInventoryComponent::GetCooldownReduction() const
{
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return FMath::Clamp(ASC->GetNumericAttribute(USnAttributeSet::GetCooldownReductionAttribute()), 0.f, 100.f);
	}
	return 0.f;
}

// --- Hotbar queries (UI) ---

FName USnInventoryComponent::GetHotbarSlot(int32 SlotIndex) const
{
	// IsValidIndex covers both "index exists" and "not negative" cases;
	// empty slots read NAME_None straight from the array.
	if (!HotbarSlots.IsValidIndex(SlotIndex))
	{
		return NAME_None;
	}
	return HotbarSlots[SlotIndex];
}

FName USnInventoryComponent::GetSelectedHotbarRowName() const
{
	if (!HotbarSlots.IsValidIndex(SelectedHotbarIndex))
	{
		return NAME_None;
	}
	return HotbarSlots[SelectedHotbarIndex];
}

float USnInventoryComponent::GetLocalUseCooldownRemaining(FName ItemRowName) const
{
	const FSnLocalItemCooldown* Local = LocalUseCooldowns.Find(ItemRowName);
	if (!Local)
	{
		return 0.f;
	}
	if (UWorld* World = GetWorld())
	{
		const float Remaining = Local->EndTime - World->GetTimeSeconds();
		if (Remaining > 0.f)
		{
			return Remaining;
		}
	}
	return 0.f;
}

float USnInventoryComponent::GetLocalUseCooldownFraction(FName ItemRowName) const
{
	const FSnLocalItemCooldown* Local = LocalUseCooldowns.Find(ItemRowName);
	if (!Local || Local->TotalDuration <= 0.f)
	{
		return 0.f;
	}
	if (UWorld* World = GetWorld())
	{
		const float Remaining = Local->EndTime - World->GetTimeSeconds();
		if (Remaining > 0.f)
		{
			// Clamped: the mirror's duration and the live clock could drift a
			// hair past the ends on frame boundaries.
			return FMath::Clamp(Remaining / Local->TotalDuration, 0.f, 1.f);
		}
	}
	return 0.f;
}

// --- Queries ---

int32 USnInventoryComponent::GetInventoryCapacity() const
{
	// Capacity is a replicated GAS attribute: expansion items add to it via
	// GE modifiers (Instant = permanent, Infinite = while equipped). The
	// attribute is authoritative - the slot array may temporarily exceed it
	// (failed shrink) and ResizeToCapacity compacts/truncates as items leave.
	if (const UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		return FMath::Max(1, (int32)FMath::Floor(ASC->GetNumericAttribute(USnAttributeSet::GetCapacityAttribute())));
	}
	return DefaultCapacity;
}

bool USnInventoryComponent::IsInventoryFull() const
{
	for (const FSnItemStack& Stack : Inventory)
	{
		if (Stack.ItemRowName.IsNone())
		{
			return false;
		}
	}
	return true;
}

int32 USnInventoryComponent::GetItemCount(FName ItemRowName) const
{
	return USnItemLibrary::CountItemInStacks(Inventory, ItemRowName);
}

FName USnInventoryComponent::GetItemRowNameAt(int32 SlotIndex) const
{
	// IsValidIndex covers both the "index exists" and "not negative" cases;
	// empty slots read NAME_None straight from the stack.
	if (!Inventory.IsValidIndex(SlotIndex))
	{
		return NAME_None;
	}
	return Inventory[SlotIndex].ItemRowName;
}

bool USnInventoryComponent::HasItem(FName ItemRowName, int32 Count) const
{
	return GetItemCount(ItemRowName) >= Count;
}

FSnGridItem USnInventoryComponent::GetGridItemAt(int32 GridIndex) const
{
	const int32 Index = FindGridItemIndex(GridIndex);
	return Index == INDEX_NONE ? FSnGridItem() : EquipmentGrid[Index];
}

bool USnInventoryComponent::IsGridCellOccupied(int32 GridIndex) const
{
	return FindGridItemIndex(GridIndex) != INDEX_NONE;
}

int32 USnInventoryComponent::GetGridCellLevel(int32 GridIndex) const
{
	// Derived, pure: the sum of every covering rune shape (position +
	// rotation), boundary-culled in GetCoveredCells. No stored state, so the
	// level can never drift from the layout - moves and rotations recompute
	// for free. Stacks: several runes boosting the same cell add up.
	int32 CellLevel = 0;
	TArray<int32> Covered;
	for (const FSnGridItem& Item : EquipmentGrid)
	{
		const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
		if (!Row || Row->RuneShapeID.IsNone())
		{
			continue; // Not a rune: boosts nothing.
		}
		Covered.Reset();
		USnItemLibrary::GetCoveredCells(Item, GridSize, Covered);
		if (Covered.Contains(GridIndex))
		{
			++CellLevel;
		}
	}
	return CellLevel;
}

int32 USnInventoryComponent::GetEffectiveItemLevel(int32 GridIndex) const
{
	const FSnGridItem Item = GetGridItemAt(GridIndex);
	if (!Item.IsValid())
	{
		return 0;
	}
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName);
	const int32 RowLevel = Row ? FMath::Max(0, Row->Level) : 0;
	// Additive cell bonus: EffectiveLevel = Row.Level + GridCellLevel (the
	// same formula RecomputeGridEffects applies with).
	return FMath::Max(0, RowLevel + GetGridCellLevel(GridIndex));
}

// --- Replication callbacks ---

void USnInventoryComponent::OnRep_Inventory()
{
	OnInventoryChanged.Broadcast();
}

void USnInventoryComponent::OnRep_EquipmentGrid()
{
	OnEquipmentChanged.Broadcast();
}

void USnInventoryComponent::OnRep_HotbarSlots()
{
	OnHotbarChanged.Broadcast();
}

void USnInventoryComponent::OnRep_SelectedHotbarIndex()
{
	OnHotbarChanged.Broadcast();
}

void USnInventoryComponent::OnRep_Gold(int64 OldGold)
{
	// Client path: the replicated value just landed. Broadcast with the applied
	// delta so widgets can show "+120" feedback on both sides of the wire.
	OnGoldChanged.Broadcast(Gold, Gold - OldGold);
}

// --- Internals ---

int32 USnInventoryComponent::FindGridItemIndex(int32 GridIndex) const
{
	for (int32 Index = 0; Index < EquipmentGrid.Num(); ++Index)
	{
		if (EquipmentGrid[Index].GridIndex == GridIndex)
		{
			return Index;
		}
	}
	return INDEX_NONE;
}

bool USnInventoryComponent::WouldExceedCapacityAfterRemoval(const FSnGridItem& Item) const
{
	// Only capacity gear (a positive effective Capacity modifier) can shrink
	// the attribute when its equip effect goes away.
	int32 PostCapacity = GetInventoryCapacity();
	bool bShrinksCapacity = false;
	if (const FSnItemRow* Row = USnItemLibrary::FindItemRow(Item.ItemRowName))
	{
		for (const FSnItemStatModifier& Modifier : Row->EquipModifiers)
		{
			if (Modifier.Attribute != USnAttributeSet::GetCapacityAttribute())
			{
				continue;
			}
			// Effective value at the item's EFFECTIVE level - the same number
			// RecomputeGridEffects fed into SetByCaller.
			const float Effective = USnItemLibrary::GetEffectiveValue(Modifier, GetEffectiveItemLevel(Item.GridIndex));
			if (Effective > 0.f)
			{
				PostCapacity -= (int32)FMath::Floor(Effective);
				bShrinksCapacity = true;
			}
		}
	}
	if (!bShrinksCapacity)
	{
		return false;
	}

	int32 NonEmpty = 0;
	for (const FSnItemStack& Stack : Inventory)
	{
		if (!Stack.ItemRowName.IsNone())
		{
			++NonEmpty;
		}
	}
	return NonEmpty > FMath::Max(1, PostCapacity);
}

bool USnInventoryComponent::SpawnDropPickup(FName ItemRowName, int32 Count)
{
	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	// PlayerState -> PlayerController -> Pawn (PlayerState itself is not a pawn).
	APawn* Pawn = nullptr;
	if (const APlayerState* PlayerState = Cast<APlayerState>(Owner))
	{
		if (const APlayerController* PC = PlayerState->GetPlayerController())
		{
			Pawn = PC->GetPawn();
		}
	}
	if (!World || !Pawn)
	{
		return false;
	}

	FVector SpawnLocation = Pawn->GetActorLocation() + Pawn->GetActorForwardVector() * 150.f;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// Configured class (usually BP_ItemPickup) so drops share the enemy loot
	// visuals; falls back to the base class when unset.
	TSubclassOf<ASnItemPickup> PickupClass = ASnItemPickup::StaticClass();
	if (const USnSettings* Settings = USnSettings::Get())
	{
		if (TSubclassOf<ASnItemPickup> Configured = Settings->GetItemPickupClass())
		{
			PickupClass = Configured;
		}
	}

	// Through the pickup subsystem (same pooling + field management as enemy
	// loot). Returns false (and the caller rolls back) only at capacity.
	if (USnPickupSubsystem* PickupSystem = World->GetSubsystem<USnPickupSubsystem>())
	{
		if (PickupSystem->SpawnItemPickup(PickupClass, SpawnLocation, SpawnRotation, ItemRowName, Count))
		{
			return true;
		}
	}
	return false;
}

UAbilitySystemComponent* USnInventoryComponent::GetAbilitySystemComponent() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

void USnInventoryComponent::BindCapacityCallbacks()
{
	if (bCapacityBound)
	{
		return;
	}
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		// Fires on the server when a GE modifies the attribute and on clients
		// when OnRep_Capacity lands (REPNOTIFY re-pushes through the same path).
		ASC->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetCapacityAttribute())
			.AddUObject(this, &USnInventoryComponent::HandleCapacityChanged);
		bCapacityBound = true;
	}
}

void USnInventoryComponent::HandleCapacityChanged(const FOnAttributeChangeData& Data)
{
	ResizeToCapacity();
	OnCapacityChanged.Broadcast(GetInventoryCapacity());
	OnInventoryChanged.Broadcast(); // Slot grid changed; UI re-renders it.
}

void USnInventoryComponent::ResizeToCapacity()
{
	const int32 Target = GetInventoryCapacity();
	if (Inventory.Num() < Target)
	{
		// Grow: pad with empty slots (default-constructed = ItemRowName None).
		Inventory.SetNum(Target);
	}
	else if (Inventory.Num() > Target)
	{
		// Shrink: compact first (items to the front, empties to the back), then
		// truncate only when everything beyond the target is empty. Otherwise
		// keep the longer array - no slots or items are lost; removals re-enter
		// here (ResizeToCapacity on RemoveItem/RemoveItemAt) so it shrinks
		// once the item count drops to the capacity.
		USnItemLibrary::SortStacks(Inventory);
		int32 NonEmpty = 0;
		for (const FSnItemStack& Stack : Inventory)
		{
			if (!Stack.ItemRowName.IsNone())
			{
				++NonEmpty;
			}
		}
		if (NonEmpty <= Target)
		{
			Inventory.SetNum(Target);
		}
	}
}
