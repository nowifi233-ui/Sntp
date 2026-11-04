// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "ActiveGameplayEffectHandle.h"
#include "CoreMinimal.h"
#include "Inventory/FSnItemTypes.h"
#include "GameplayAbilitySpecHandle.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "USnItemLibrary.generated.h"

class UAbilitySystemComponent;
class USnInventoryComponent;

/**
 * Shared lookup helpers for the item system. Gameplay and UI both resolve
 * item rows through this library, so the data table location stays in
 * USnSettings (one place) and missing rows degrade to safe values instead
 * of crashes.
 */
UCLASS()
class SN_API USnItemLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** C++ lookup: row for an item ID, or nullptr if unknown. */
	static const FSnItemRow* FindItemRow(FName ItemRowName);

	/** C++ lookup: row + validation for UI/hud helpers. */
	static const FSnItemRow* FindItemRowChecked(FName ItemRowName, bool& bFound);

	/** C++ lookup: bond row by tag (row name = tag name), or nullptr if
	 *  unknown. */
	static const FSnBondRow* FindBondRow(FGameplayTag BondTag);

	/** Every item row name in the item data table, in table row order (the
	 *  IDs GiveItem / AddItem accept). The developer cheat panel builds its
	 *  list from this. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static TArray<FName> GetAllItemRowNames();

	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText GetItemDisplayName(FName ItemRowName);

	/** "Iron Sword" for count 1, "Health Potion ×3" otherwise. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText GetStackDisplayName(FName ItemRowName, int32 Count);

	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText GetItemDescription(FName ItemRowName);

	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static TSoftObjectPtr<UTexture2D> GetItemIcon(FName ItemRowName);

	/** Synchronously loaded icon. Blueprint functions cannot host the
	 *  "Load Soft Object" node (EventGraph-only), so loading moves into C++;
	 *  this returns the hard reference usable anywhere (slots, functions). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static UTexture2D* GetItemIconLoaded(FName ItemRowName);

	/** All capability tags of the item (empty for missing rows). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FGameplayTagContainer GetItemTags(FName ItemRowName);

	/** All bond tags of the item's row (empty for missing rows). UI reads
	 *  this for the item detail panel; the ACTIVE bonds (equipped count) come
	 *  from USnInventoryComponent::GetActiveBonds. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FGameplayTagContainer GetItemBonds(FName ItemRowName);

	/** Whether the item carries Tag (subtree match, same semantics the action
	 *  table uses). Replaces the old single-type GetItemType. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static bool HasItemTag(FName ItemRowName, const FGameplayTag& Tag);

	/** Whether any tag in Container matches Tag (subtree: Tag or a parent of a
	 *  container tag). Shared by equip eligibility and action filtering. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static bool HasAnyMatchingTag(const FGameplayTagContainer& Container, const FGameplayTag& Tag);

	/** The item's level (equipment scales its modifiers with it, GA growth;
	 *  consumables/materials just display it). Clamped to >= 1. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static int32 GetItemLevel(FName ItemRowName);

	/** The item's quality tier (the row's Quality column; Common for missing
	 *  rows). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static ESnItemQuality GetItemQuality(FName ItemRowName);

	/** UI style of a quality tier (row name = the enum name in the quality
	 *  style table). Missing table/row falls back to the built-in default:
	 *  white name, no background. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FSnQualityStyleRow GetQualityStyle(ESnItemQuality Quality);

	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static int32 GetItemStackSize(FName ItemRowName);

	/** The item's sell price in gold (FSnItemRow::SellPrice). 0 = not sellable
	 *  AND the detail panel hides the price line - the price IS the permission.
	 *  Missing rows/tables degrade to 0 (not sellable) like every other lookup
	 *  here. Fixed value: no level or rune-bonus scaling. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static int32 GetSellPrice(FName ItemRowName);

	/** Whether the row exists (drives "equip vs use vs inert" UI decisions). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static bool IsValidItem(FName ItemRowName);

	// --- Slot-array operations shared by the bag and storage containers ---
	// Fixed-slot model: the array length IS the capacity; empty slots are
	// FSnItemStack with ItemRowName == NAME_None. Callers keep the array
	// length in sync with the capacity (see ResizeToCapacity).

	/** Auto-stacks into same-ID slots first, then into empty slots.
	 *  Returns the count actually added (0 when no room). */
	static int32 AddItemToStacks(TArray<FSnItemStack>& Stacks, FName ItemRowName, int32 Count);

	/** Removes Count across slots (all-or-nothing: fails without changes when
	 *  the slots don't hold enough). Depleted slots become empty, they are not
	 *  removed from the array. Returns whether the removal succeeded. */
	static bool RemoveItemFromStacks(TArray<FSnItemStack>& Stacks, FName ItemRowName, int32 Count);

	/** Removes Count from the single slot at Index. Slot-exact: fails (nothing
	 *  removed) when the slot is out of range or holds fewer than Count - it
	 *  never spills across slots. Used by UI-click operations (use/equip/drop/
	 *  transfer) so they target the clicked group, not the last matching stack. */
	static bool RemoveItemFromStackAt(TArray<FSnItemStack>& Stacks, int32 Index, int32 Count);

	/** Total count of ItemRowName across the slots. */
	static int32 CountItemInStacks(const TArray<FSnItemStack>& Stacks, FName ItemRowName);

	/** One-click sort: merges same-ID slots, then re-packs everything into full
	 *  stacks ordered by row name at the front of the array (stable,
	 *  deterministic). The array length never changes - trailing slots empty
	 *  out. Shared by the bag and storage containers. */
	static void SortStacks(TArray<FSnItemStack>& Stacks);

	/** Detail-panel stat lines for an item: reads the row's EquipModifiers and
	 *  renders each effective value as "AttackPower +14" style text via the
	 *  attribute catalog's display names (single source of truth - no duplicate
	 *  config). Shows the FINAL value at Level - never the raw per-level
	 *  breakdown. Empty for items without equip modifiers.
	 *  Level is the item's EFFECTIVE level: the row level for bag slots, but on
	 *  the equipment grid Row.Level + the covering-rune bonus (callers get it
	 *  from USnInventoryComponent::GetEffectiveItemLevel) - the same level the
	 *  equip GE values were applied with, so the panel always matches the
	 *  gameplay numbers. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetEquipStatLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines);

	/** Same rendering as GetEquipStatLines, but for UseModifiers: shows a
	 *  consumable's effective values (e.g. Health +50) at Level. Consumables
	 *  live in bag slots, so Level is normally just GetItemLevel(ItemRowName).
	 *  Empty for items without use modifiers. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetUseStatLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines);

	/** Formats the item's EQUIP description template (FSnItemRow::
	 *  EquipDescription): {Attribute.X} named placeholders are filled with the
	 *  EFFECTIVE modifier values at Level, matched by attribute tag - order-
	 *  independent, so editing modifier order never breaks the text.
	 *  Rich-text tags (<Attribute.Health>word</>) pass through untouched for
	 *  RichTextBlock. Missing placeholders stay literal so template/row
	 *  mismatches are visible. Empty template = empty text (the segment then
	 *  shows only the auto-generated stat lines). The background lore comes
	 *  from GetItemDescription (verbatim, no placeholders). Level follows the
	 *  same rule as GetEquipStatLines: effective level, rune bonus included on
	 *  the equipment grid. Full syntax: Docs/DESCRIPTION_TEMPLATE.md. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText FormatEquipDescription(FName ItemRowName, int32 Level);

	/** Description lines for the abilities the equipment grants
	 *  (FSnItemRow::EquipAbilities): one line per granted ability, rendered as
	 *  "AbilityName: text". The name comes from the ability catalog
	 *  (USnAbilityCatalog - fallback to the class display name when the entry
	 *  is missing); the text is the ability's own GetDescription (blueprint
	 *  overrides generate level-scaled numbers, otherwise the static
	 *  Description field). Abilities without text are skipped. Level follows
	 *  FormatEquipDescription: the item's EFFECTIVE level (row level + rune
	 *  bonus on the equipment grid) - the same level GrantEquipAbilities gave
	 *  the ability spec, so the panel always matches the actual granted
	 *  ability. Empty when the item has no equip abilities or nothing to
	 *  describe (the UI hides the ability section). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetEquipAbilityDescriptionLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines);

	/** Same as FormatEquipDescription, but for the USE segment
	 *  (FSnItemRow::UseDescription), filled with the use modifier values at
	 *  Level. Items with both equip and use segments call both; the UI decides
	 *  which segments to show from the item tags. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText FormatUseDescription(FName ItemRowName, int32 Level);

	/** Description line for the ability the item grants on USE
	 *  (FSnItemRow::UseAbility - a single ability, unlike the equip array):
	 *  one "AbilityName: text" line with the same rendering as
	 *  GetEquipAbilityDescriptionLines (catalog display name, fallback to the
	 *  class display name; text from the ability's own GetDescription at
	 *  Level). Empty when the item has no use ability or nothing to describe
	 *  (the UI hides the use-ability line). Level follows the equip twin's
	 *  rule: the level the use ability was granted at - ServerUseItem grants
	 *  FMath::Max(1, Row->Level), effective level (rune bonus included) when
	 *  the use comes from a grid slot. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetUseAbilityDescriptionLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines);

	// --- Bond (羁绊) row queries (FSnBondRow, row name = bond tag name) ---
	// Pure table data - no instance state; the item-count side (level, owned
	// tags) lives on USnInventoryComponent.

	/** The bond's display name (FSnBondRow::DisplayName); empty when the row
	 *  is missing, so the UI renders nothing instead of a fallback name. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText GetBondDisplayName(FGameplayTag BondTag);

	/** The bond's MAX level: the LARGEST tier Count in its row (tiers 2/4/6 =
	 *  max level 6 - the piece count that maxes the bond), 0 when the row is
	 *  missing or has no tiers. The UI shows "level x/y" progress with
	 *  USnInventoryComponent::GetBondLevel. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static int32 GetBondMaxLevel(FGameplayTag BondTag);

	/** Per-tier description array (one entry per tier, ascending; index N
	 *  belongs to tier N, same order as GetBondTierStatLines). Each entry is
	 *  "(Count) " prefixed with the tier's required piece count, followed by
	 *  the tier's Description template filled with that tier's EFFECTIVE
	 *  modifier values - the same {Attribute.X} placeholder mechanism as the
	 *  item descriptions, so bond effects read like item effects
	 *  ("(2) 生命值 +20"). Tiers without a description yield just the count
	 *  prefix (they show only their stat lines). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetBondTierDescriptions(FGameplayTag BondTag, TArray<FText>& OutDescriptions);

	/** Per-tier stat-line array (one entry per tier, ascending; each entry is
	 *  the tier's modifiers rendered as "AttackPower +14" lines, newline-
	 *  joined so the array aligns 1:1 with GetBondTierDescriptions - the UI
	 *  pairs index N of both for one tier's effect block). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetBondTierStatLines(FGameplayTag BondTag, TArray<FText>& OutStatLines);

	/** Per-tier PASSIVE-ABILITY description array (one entry per tier,
	 *  ascending; each entry is the tier's EquipAbility rendered as
	 *  "AbilityName: text" - the name from the ability catalog (class display
	 *  name fallback), the text the ability's own GetDescription at Level 1
	 *  (the level RecomputeBonds grants tier abilities at), newline-joined so
	 *  the array aligns 1:1 with GetBondTierDescriptions / GetBondTierStatLines
	 *  - the UI pairs index N of all three for one tier's effect block.
	 *  Tiers without an ability (or whose ability has no text) yield an empty
	 *  entry. Covers the ignite-style tiers whose numbers live on the GA CDO,
	 *  not in the tier's stat modifiers. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetBondTierAbilityLines(FGameplayTag BondTag, TArray<FText>& OutAbilityLines);

	/** The tiers' required piece counts, ascending (the row's Tiers, e.g.
	 *  [2, 4, 6]). Same order as GetBondTierDescriptions / GetBondTierStatLines.
	 *  Empty for missing rows. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetBondTierCounts(FGameplayTag BondTag, TArray<int32>& OutCounts);

	/** Description of the tier whose required piece count equals Count
	 *  (exact match; tiers are ascending by Count), formatted exactly like a
	 *  GetBondTierDescriptions entry: "(Count) description". Empty text when
	 *  no tier needs Count pieces or the row is missing. Lets the UI pull one
	 *  tier's text by its count instead of walking the arrays. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText GetBondDescriptionByCount(FGameplayTag BondTag, int32 Count);

	/** The tier's PASSIVE-ABILITY line whose required piece count equals Count
	 *  (exact match; tiers are ascending by Count), formatted exactly like a
	 *  GetBondTierAbilityLines entry: "AbilityName: text". Empty text when no
	 *  tier needs Count pieces, the row is missing, or the tier grants no
	 *  ability. By-count counterpart of GetBondDescriptionByCount - the UI
	 *  pulls one tier's ability text (e.g. the ignite passive) without walking
	 *  the arrays. */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FText GetBondAbilityLineByCount(FGameplayTag BondTag, int32 Count);

	/** The bond's icon (FSnBondRow::Icon; null soft ptr when missing). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static TSoftObjectPtr<UTexture2D> GetBondIcon(FGameplayTag BondTag);

	/** Filters the item action table (USnSettings::ItemActionDataTable) by the
	 *  clicked slot's item tags + source + storage-open state, in table row
	 *  order. Empty RequiredItemType/RequiredSource = any; an item matches when
	 *  ANY of its tags matches RequiredItemType (subtree), Source exactly. The
	 *  menu only shows actions that pass; an empty result = "don't open menu". */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static void GetAvailableItemActions(FGameplayTagContainer ItemTags, FGameplayTag SourceTag, bool bStorageOpen, TArray<FSnItemActionInfo>& OutActions);

	/** Runs one item action: looks up its row by ActionRowName, instantiates
	 *  the row's ActionClass and calls Execute with the given context. No-op
	 *  when the row or the action class is missing. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Items")
	static void ExecuteItemAction(FName ActionRowName, USnInventoryComponent* Inventory, USnStorageComponent* Storage, const FSnItemActionContext& Context);

	// --- Crafting recipes (USnSettings::CraftRecipeDataTable) ---

	/** C++ lookup: recipe row for a recipe ID, or nullptr if unknown. */
	static const FSnCraftRecipeRow* FindRecipeRow(FName RecipeRowName);

	/** All recipes in the crafting data table (table row order). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static TArray<FSnCraftRecipeRow> GetAllCraftRecipes();

	/** One recipe row by ID; default (empty Materials) when unknown - the UI
	 *  treats an empty Materials list as "invalid, hide". */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static FSnCraftRecipeRow GetCraftRecipe(FName RecipeRowName);

	/** How many times the recipe can be crafted right now: the minimum of
	 *  (held material count / required count) over all ingredients. 0 = not
	 *  enough materials (UI disables the craft button). */
	UFUNCTION(BlueprintPure, Category = "Sn|Items")
	static int32 GetAvailableCraftCount(const FSnCraftRecipeRow& Recipe, const USnInventoryComponent* Inventory);

	// --- Server-side application helpers (USnInventoryComponent calls these) ---

	/** Applies Modifiers through EffectClass via SetByCaller (key = the
	 *  attribute's "Attribute.X" tag, value = effective value at Level).
	 *  DurationSeconds > 0 overrides the effect's duration (the buff GE takes
	 *  its length from the item row). Returns the active handle; invalid when
	 *  the ASC/class is missing or the spec could not be built. */
	static FActiveGameplayEffectHandle ApplyItemModifiers(
		UAbilitySystemComponent* ASC,
		const TSubclassOf<UGameplayEffect>& EffectClass,
		const TArray<FSnItemStatModifier>& Modifiers,
		int32 Level,
		float DurationSeconds = 0.f);

	/** Grants every ability in EquipAbilities at Level (GA growth) and
	 *  activates those opting in (bActivateOnEquip on USnGameplayAbility) -
	 *  equipping is grant + activate in one, owned by this flow. Records the
	 *  granted ability handles in OutHandles for revocation on the next
	 *  recompute (unequip / grid replay). */
	static void GrantEquipAbilities(UAbilitySystemComponent* ASC, const TArray<TSubclassOf<UGameplayAbility>>& EquipAbilities, int32 Level, TArray<FGameplayAbilitySpecHandle>& OutHandles);

	// --- Rune shapes (USnSettings::RuneDataTable) ---

	/** C++ lookup: shape row for a rune shape ID, or nullptr if unknown. */
	static const FSnRuneShapeRow* FindRuneShapeRow(FName RuneShapeID);

	/** Rotates the shape's offsets one clockwise 90-degree step per rotation
	 *  (screen convention: X = column +right, Y = row +down, so a cell right
	 *  of the anchor lands below it after one step - the visual rotation a
	 *  player expects). Any step count normalizes to mod 4. */
	static void RotateOffsets(const TArray<FIntPoint>& InOffsets, int32 RotationSteps, TArray<FIntPoint>& OutOffsets);

	/** Absolute grid indices covered by the item's rune shape around its own
	 *  cell (GridIndex), after the item's rotation, culled to the GridSize x
	 *  GridSize bounds (a corner cross covers fewer cells). Empty when the
	 *  item is not a rune (no RuneShapeID) or has no offsets. */
	static void GetCoveredCells(const FSnGridItem& Item, int32 GridSize, TArray<int32>& OutGridIndices);

	/** Effective magnitude of a modifier at the given item level:
	 *  Value + Level * ValuePerLevel (levels start at 0 - level 0 IS the base
	 *  value). Floors negative levels to 0. */
	static float GetEffectiveValue(const FSnItemStatModifier& Modifier, int32 Level);

private:
	/** Shared line rendering for both stat-line helpers: walks the modifier
	 *  array and formats each effective value as "DisplayName +14" (catalog
	 *  display name; lines without a catalog entry are skipped, near-zero
	 *  values are skipped too). */
	static void AppendStatLines(const TArray<FSnItemStatModifier>& Modifiers, int32 Level, TArray<FText>& OutLines);

	/** Shared placeholder fill behind both description segments: fills
	 *  {Attribute.X} placeholders in Template with the EFFECTIVE values of the
	 *  given modifiers (matched by attribute tag - order-independent).
	 *  Placeholders without a matching modifier stay literal so template/row
	 *  mismatches are visible. Empty template = empty text. */
	static FText FillDescriptionTemplate(const FText& Template, const TArray<FSnItemStatModifier>& Modifiers, int32 Level);

	/** Shared rendering behind the per-tier description outputs: fills the
	 *  tier's Description template with its EFFECTIVE modifier values and
	 *  prefixes it with "(Count) " (the required piece count). */
	static FText FormatBondTierDescription(const FSnBondTier& Tier);

	/** The attribute's SetByCaller magnitude key: "SetByCaller.<AttributeName>"
	 *  (registered in SnGameplayTags or Project Settings > Gameplay Tags) -
	 *  the GE editor's SetByCaller tag picker only offers tags under the
	 *  SetByCaller root, so "Attribute.X" tags cannot be selected there.
	 *  Invalid when the tag is unregistered. */
	static FGameplayTag GetSetByCallerTag(const FGameplayAttribute& Attribute);

	/** C++ lookup: row for an item action ID, or nullptr if unknown. */
	static const FSnItemActionRow* FindActionRow(FName ActionRowName);
};
