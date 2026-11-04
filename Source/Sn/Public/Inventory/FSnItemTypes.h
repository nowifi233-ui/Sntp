// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "AttributeSet.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateColor.h"
#include "FSnItemTypes.generated.h"

class UGameplayAbility;
class UGameplayEffect;
class UStaticMesh;
class UItemAction;

/** Item quality tier: drives display color/background (quality style table)
 *  and sorting eventually. Ordinal order matters
 *  (Common < Uncommon < ... < Legendary). */
UENUM(BlueprintType)
enum class ESnItemQuality : uint8
{
	Common UMETA(DisplayName = "普通"),
	Uncommon UMETA(DisplayName = "优秀"),
	Rare UMETA(DisplayName = "稀有"),
	Epic UMETA(DisplayName = "史诗"),
	Legendary UMETA(DisplayName = "传说"),
};

/**
 * One row in the quality style data table (USnSettings::
 * QualityStyleDataTable). Row name = the quality enum name (Common /
 * Uncommon / Rare / Epic / Legendary - same "row name = ID" convention as the
 * enemy/item tables, so C++ looks up by FName directly). Pure display data:
 * what an item of that quality looks like in the UI (slot background +
 * detail-panel name color). Missing rows degrade to the built-in default
 * (white name, no background) - the UI stays usable without the table.
 */
USTRUCT(BlueprintType)
struct FSnQualityStyleRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Item name color in the detail panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
	FSlateColor NameColor;

	/** Slot background color when Background has no texture (pure-color
	 *  fallback). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
	FSlateColor BorderColor;

	/** Optional slot background texture; when set it replaces the pure-color
	 *  background (the texture carries its own colors). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quality")
	TSoftObjectPtr<UTexture2D> Background;
};

/**
 * One stat modifier entry on an item row: an attribute plus the value granted.
 * Equipment scales with the item's level
 * (EffectiveValue = Value + Level * ValuePerLevel - levels start at 0, level 0
 * = the base value); consumables use the
 * same formula with ValuePerLevel usually 0. The effective value is fed into a
 * generic GameplayEffect via SetByCaller (key = the attribute's "Attribute.X"
 * tag) - items never carry their own effect classes anymore.
 */
USTRUCT(BlueprintType)
struct FSnItemStatModifier
{
	GENERATED_BODY()

	/** Attribute the modifier affects (Health, MaxHealth, AttackPower, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FGameplayAttribute Attribute;

	/** Effective value at level 0 (the row's base value). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float Value = 0.f;

	/** Extra effective value per item level above 0 (equipment growth). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	float ValuePerLevel = 0.f;
};

/**
 * One row in the global item data table (USnSettings::ItemDataTable).
 * The row name is the item's ID, e.g. "IronSword". Pure static data -
 * runtime state lives in FSnItemStack, so items stay FStruct-friendly
 * (no UObject instances: GC-safe, replication-safe, trivially savable).
 *
 * Panel layout: the row editor shows one collapsible group per Category, in
 * the order the categories first appear here, and declaration order inside
 * each group. Fields are grouped as 基础 / 装备 / 使用 / 通用 so an author
 * reads top-down: identity and tags first, then only the blocks the item
 * actually uses. Rows are serialized by property NAME, so regrouping and
 * reordering never touches existing assets (CSV column order follows the
 * declaration order; import matches by column name).
 */
USTRUCT(BlueprintType)
struct FSnItemRow : public FTableRowBase
{
	GENERATED_BODY()

	// --- 基础: identity, display and classification -------------------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "名称"))
	FText DisplayName;

	/** Background lore, shown above the effect segments. Pure text - the
	 *  effect numbers live in the modifier stat lines, not here. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "背景描述"))
	FText Description;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "图标"))
	TSoftObjectPtr<UTexture2D> Icon;

	/** In-world mesh shown while this item is SELECTED in the hotbar: attached
	 *  to the character's HandSocket (world transform). Empty = fall back to
	 *  the icon billboard (a two-sided icon card showing Icon). Pure display -
	 *  the held visual is derived locally from the replicated selection, never
	 *  replicated itself. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "世界模型"))
	TSoftObjectPtr<UStaticMesh> WorldMesh;

	/** Quality tier (display color later, sorting eventually). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "品质"))
	ESnItemQuality Quality = ESnItemQuality::Common;

	/** The item's level. Equipment scales its modifier values with it
	 *  (EffectiveValue = Value + Level * ValuePerLevel - levels start at 0,
	 *  level 0 = the base values); consumables and
	 *  materials just display it. On the equipment grid the CELL level (sum of
	 *  the covering rune shapes) adds on top: the effective level = row level
	 *  + cell level, and it feeds the equip GE values and the GA growth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "等级"))
	int32 Level = 0;

	/** Item capability tags: what the item can do / is used for
	 *  (Item.Type.Equipment / .Consumable / .Material / .Rune). An item may
	 *  carry several - the right-click menu shows every action matching any
	 *  tag.
	 *  These tags are ALSO the only thing that drives the panel: the 装备 and
	 *  使用 blocks below appear only for rows tagged for them (see
	 *  SyncPanelSwitches), so tag first, then fill the block that shows up. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "物品类型标签", Categories = "Item.Type"))
	FGameplayTagContainer ItemTags;

	/** Bond tags: an item may belong to several bonds at once (e.g. both
	 *  "Item.Bond.Fire" and "Item.Bond.Warrior"). Only items EQUIPPED on the
	 *  grid count toward a bond (bag items are dormant); each bond applies
	 *  its highest satisfied tier to the player (see FSnBondRow - the table
	 *  row name IS the tag name). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "基础", meta = (DisplayName = "羁绊标签", Categories = "Item.Bond"))
	FGameplayTagContainer BondTags;

	// --- 装备: only items that go on the equipment grid ---------------------
	// Shown when the row carries Item.Type.Equipment or Item.Type.Rune: both
	// types are placed on the grid and both run through ApplyItemToASC
	// (equip modifiers + equip abilities), so they share the block.

	/** Optional flavor text for the equip segment (items with
	 *  Item.Type.Equipment). Same {Attribute.X} placeholder template as the
	 *  old single Description: filled with the EFFECTIVE EquipModifiers
	 *  values, matched by attribute tag. Empty = the segment shows only the
	 *  auto-generated stat lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备", meta = (DisplayName = "装备描述", EditCondition = "bPanelHasEquipBlock", EditConditionHides, HideEditConditionToggle))
	FText EquipDescription;

	/** Equipment only: modifiers applied on equip through the generic equip GE
	 *  (SetByCaller, key = the attribute's "Attribute.X" tag). No per-item GE
	 *  assets anymore - the generic GE from Sn Settings carries the modifiers,
	 *  this list carries the values. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备", meta = (DisplayName = "装备属性", EditCondition = "bPanelHasEquipBlock", EditConditionHides, HideEditConditionToggle))
	TArray<FSnItemStatModifier> EquipModifiers;

	/** Equipment only: abilities granted while equipped (given at the item's
	 *  Level), revoked on unequip. The equip modifiers always ride the
	 *  generic equip GE from Sn Settings - this array only grants abilities
	 *  (e.g. a sustain-trigger aura). One class picker per cell (the same TSubclassOf
	 *  editing as the action rows). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备", meta = (DisplayName = "装备能力", EditCondition = "bPanelHasEquipBlock", EditConditionHides, HideEditConditionToggle))
	TArray<TSubclassOf<UGameplayAbility>> EquipAbilities;

	/** Rune only: row name in the rune shape data table (USnSettings::
	 *  RuneDataTable) - the pattern of cells the rune boosts around its own
	 *  cell. Empty for non-rune items. The shape is rotated by the item's
	 *  RotationSteps at placement (see FSnRuneShapeRow).
	 *  Its own, stricter switch: it sits in the equip block but only a rune
	 *  ever fills it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "装备", meta = (DisplayName = "石板形状", EditCondition = "bPanelHasRuneField", EditConditionHides, HideEditConditionToggle))
	FName RuneShapeID = NAME_None;

	// --- 使用: only items that get used from the bag/hotbar -----------------
	// Shown when the row carries Item.Type.Consumable.

	/** Optional flavor text for the use segment (items with
	 *  Item.Type.Consumable). Placeholders filled with the EFFECTIVE
	 *  UseModifiers values. Empty = stat lines only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "使用", meta = (DisplayName = "使用描述", EditCondition = "bPanelHasUseBlock", EditConditionHides, HideEditConditionToggle))
	FText UseDescription;

	/** Consumable only: modifiers applied on use. Same SetByCaller mechanism
	 *  as EquipModifiers. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "使用", meta = (DisplayName = "使用属性", EditCondition = "bPanelHasUseBlock", EditConditionHides, HideEditConditionToggle))
	TArray<FSnItemStatModifier> UseModifiers;

	/** Consumable only: > 0 routes the use through the generic buff GE with
	 *  this duration (seconds); 0 = instant use GE. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "使用", meta = (DisplayName = "生效时长", EditCondition = "bPanelHasUseBlock", EditConditionHides, HideEditConditionToggle))
	float UseDurationSeconds = 0.f;

	/** Use cooldown in seconds (0 = no cooldown). Per item ID: using one
	 *  stack of a type locks every stack of that type, other items are
	 *  unaffected (independent per-item cooldowns - no shared Use tag). The
	 *  length rides the shared cooldown GE from Sn Settings
	 *  (UseCooldownEffectClass) as a SetByCaller duration, scaled by the
	 *  player's CooldownReduction attribute at use time (server-authoritative);
	 *  clients mirror the same formula for the UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "使用", meta = (DisplayName = "使用冷却", ClampMin = "0.0", EditCondition = "bPanelHasUseBlock", EditConditionHides, HideEditConditionToggle))
	float UseCD = 0.f;

	/** Ability the item's use runs ("use also carries one ability"), given at
	 *  the item's Level. HOW it is given depends on where the use came from:
	 *  the hotbar holds it while the item is the selected slot (granted on
	 *  selection, revoked with it, activated per use - the held window), while
	 *  a use from the bag or an item action grants it for that one use through
	 *  the engine one-shot API. Empty = the use applies UseModifiers only. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "使用", meta = (DisplayName = "使用能力", EditCondition = "bPanelHasUseBlock", EditConditionHides, HideEditConditionToggle))
	TSubclassOf<UGameplayAbility> UseAbility;

	/** Whether using the item consumes one from the stack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "使用", meta = (DisplayName = "使用时消耗", EditCondition = "bPanelHasUseBlock", EditConditionHides, HideEditConditionToggle))
	bool bConsumeOnUse = true;

	// --- 通用: shared by every item kind ------------------------------------

	/** Max count per slot (1 for equipment, >1 for consumables/materials). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "通用", meta = (DisplayName = "堆叠上限"))
	int32 StackSize = 1;

	/** Whether the player may drop the item into the world. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "通用", meta = (DisplayName = "可丢弃"))
	bool bCanDrop = true;

	/** Sell price in gold. > 0 = the item can be sold AND its "售价" line shows
	 *  in the detail panel; 0 = not sellable, no price shown. One field is both
	 *  the permission and the number on purpose - a separate bSellable flag
	 *  could only ever drift out of sync with the price.
	 *  Fixed value: it does NOT scale with the item's level, and NOT with the
	 *  equipment grid's rune-boosted effective level (otherwise runes would be
	 *  a money printer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "通用", meta = (DisplayName = "售价", ClampMin = "0"))
	int32 SellPrice = 0;

	/** Panel-only switches, recomputed from ItemTags by SyncPanelSwitches.
	 *  Gameplay NEVER reads them - ItemTags stays the single source of truth -
	 *  they exist because a UPROPERTY edit condition can only read plain
	 *  sibling fields: 5.8's EditConditionParser takes property tokens and
	 *  operators (no function calls, no tag containers), and the row editor is
	 *  a structure details view that runs no IDetailCustomization, so a
	 *  derived bool is the one shape a row's panel can be gated with.
	 *
	 *  They carry no Edit/Visible specifier on purpose (never shown, never
	 *  hand-ticked) and the fields they gate carry HideEditConditionToggle, so
	 *  no checkbox is drawn for a value the sync owns. They are plain
	 *  UPROPERTYs, so they save with the row and a reopened table lays its
	 *  blocks out right without touching anything. */
	UPROPERTY()
	bool bPanelHasEquipBlock = false;

	/** The rune-only field inside the equip block (RuneShapeID). */
	UPROPERTY()
	bool bPanelHasRuneField = false;

	UPROPERTY()
	bool bPanelHasUseBlock = false;

	/** Editor-time sync of the three switches above. Called on every row edit
	 *  (SRowEditor::NotifyPostChange -> UDataTable::HandleDataTableChanged ->
	 *  here) and on import, so the panel follows the tags as you type: row
	 *  visibility is evaluated live, so a block appears/disappears the moment
	 *  its tag is added or removed. Hiding never clears a value - untagging and
	 *  re-tagging brings the block back with its old configuration. */
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override
	{
		SyncPanelSwitches();
	}

	virtual void OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) override
	{
		SyncPanelSwitches();
	}

private:
	/** The one place the switches are computed: tags in, panel state out.
	 *  FGameplayTagContainer::HasTag matches parent tags too
	 *  ({"Item.Type.Equipment.Weapon"}.HasTag("Item.Type.Equipment") is true) -
	 *  the same semantics USnItemLibrary::HasAnyMatchingTag uses for gameplay,
	 *  so the panel and the grid/use paths can never disagree about what an
	 *  item is.
	 *  Rows written before these switches existed keep the default false until
	 *  they are synced: any edit of a row syncs that row, and the editor
	 *  console command Sn.ResyncItemRows syncs the whole table. */
	void SyncPanelSwitches()
	{
		bPanelHasEquipBlock = ItemTags.HasTag(SnGameplayTags::Item_Type_Equipment) || ItemTags.HasTag(SnGameplayTags::Item_Type_Rune);
		bPanelHasRuneField = ItemTags.HasTag(SnGameplayTags::Item_Type_Rune);
		bPanelHasUseBlock = ItemTags.HasTag(SnGameplayTags::Item_Type_Consumable);
	}
};

/** One bond tier: when the number of equipped grid items carrying the bond's
 *  tag reaches Count, the tier's modifiers apply to the player. Only the
 *  HIGHEST satisfied tier is active per bond (no tier stacking). */
USTRUCT(BlueprintType)
struct FSnBondTier
{
	GENERATED_BODY()

	/** Equipped bond items required to activate this tier. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	int32 Count = 2;

	/** Attributes granted while the tier is active (same SetByCaller
	 *  mechanism as the equip modifiers; values are used as-is, no level
	 *  scaling - ValuePerLevel stays 0). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	TArray<FSnItemStatModifier> Modifiers;

	/** Effect description of this tier, written as an {Attribute.X} template
	 *  like the item descriptions (same syntax, Docs/DESCRIPTION_TEMPLATE.md):
	 *  placeholders are filled with the tier's EFFECTIVE modifier values.
	 *  Empty = the tier shows only its auto-generated stat lines. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	FText Description;

	/** Passive ability granted while the tier is active (grant + auto-activate
	 *  through USnItemLibrary::GrantEquipAbilities - the same bActivateOnEquip
	 *  flow as the item rows' EquipAbilities, so the tier window IS the
	 *  ability's window; losing the tier revokes it, which cancels and
	 *  unbinds the passive). Granted at Level 1 (bond values are authored as
	 *  is, no level scaling). Empty = the tier grants its modifiers only.
	 *  Currently used by the Fire bond's tiers (the ignite passive, a
	 *  USnGA_Ignite subclass asset - per-tier strength = per-tier asset with
	 *  its own CDO values). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	TSubclassOf<UGameplayAbility> EquipAbility;
};

/**
 * One row in the bond data table (USnSettings::BondDataTable). The row name
 * IS the bond's gameplay tag name (e.g. "Item.Bond.Fire"), so items reference
 * bonds by tag and the lookup is a plain FindRow. Tiers must be ascending by
 * Count - the recompute takes the last tier whose Count the equipped total
 * meets. Pure static data (the runtime state - one GE per active bond - lives
 * in USnInventoryComponent::BondEffects).
 */
USTRUCT(BlueprintType)
struct FSnBondRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Bond name shown in UI (detail panel / progress display). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	FText DisplayName;

	/** Bond icon shown in UI (detail panel / progress display). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Activation thresholds, ascending by Count (2/4/6 pieces, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bond")
	TArray<FSnBondTier> Tiers;
};

/**
 * One runtime slot in an inventory/storage: a reference to a data table row
 * plus a stack count. The whole item system runs on these - no per-item
 * UObject instances, so arrays of stacks replicate and serialize cheaply.
 *
 * Fixed-slot model: the bag/storage arrays are ALWAYS full-length (one entry
 * per slot, the length IS the capacity). An empty slot is this struct with
 * ItemRowName == NAME_None (Count 0). Empty slots stay in the array - items
 * move in and out of slots, the array never grows or shrinks (capacity
 * changes pad/truncate trailing slots instead).
 */
USTRUCT(BlueprintType)
struct FSnItemStack
{
	GENERATED_BODY()

	/** Row name in the item data table (the item's ID); NAME_None = empty slot. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemRowName = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 Count = 1;

	bool IsValid() const { return !ItemRowName.IsNone() && Count > 0; }
};

/**
 * One occupied cell in the equipment grid (USnInventoryComponent). Sparse:
 * only filled cells are stored - the grid side length is NOT baked into the
 * storage, so a future grid resize never breaks saves (cell indices are plain
 * numbers, Row * GridSize + Column). Pure logical state: no runtime effect or
 * ability handles (those are derived per layout by RecomputeGridEffects and
 * die with the ASC on level travel), so the struct replicates, serializes and
 * compares trivially.
 */
USTRUCT(BlueprintType)
struct FSnGridItem
{
	GENERATED_BODY()

	/** Row name in the item data table (the item's ID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FName ItemRowName = NAME_None;

	/** Cell number = Row * GridSize + Column (0..35 for the 6x6 grid). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	int32 GridIndex = 0;

	/** Clockwise rotation steps (0-3). Runes change which cells they boost;
	 *  regular equipment mostly ignores it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0", ClampMax = "3"))
	uint8 RotationSteps = 0;

	/** Cell footprint - reserved for multi-cell items later; always 1 for now
	 *  (the grid placement APIs validate against these). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Width = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "1"))
	int32 Height = 1;

	bool IsValid() const { return !ItemRowName.IsNone(); }
};

/**
 * One row in the rune shape data table (USnSettings::RuneDataTable). The row
 * name is the shape's ID (e.g. "Cross"); FSnItemRow::RuneShapeID points at it.
 * The shape is the pattern of cells the rune boosts around its OWN cell: a
 * list of offsets relative to the anchor (0,0) - X = column (+right), Y = row
 * (+down). The anchor cell is only boosted when (0,0) is in the list. Cells
 * outside the grid are culled (a cross in a corner boosts 3 cells instead of
 * 5). Rotation spins the offsets clockwise, 90 degrees per step.
 */
USTRUCT(BlueprintType)
struct FSnRuneShapeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Shape name shown in editors/UI (e.g. "十字"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
	FText DisplayName;

	/** Boosted cells relative to the rune's own cell. Example: a cross that
	 *  boosts the four neighbours (no self) is {(0,-1),(0,1),(-1,0),(1,0)}. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
	TArray<FIntPoint> Offsets;
};

/**
 * One entry in an enemy's drop list (FSnEnemyInfo::Drops).
 */
USTRUCT(BlueprintType)
struct FSnDropEntry
{
	GENERATED_BODY()

	/** Row name in the item data table to spawn on death. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop")
	FName ItemRowName = NAME_None;

	/** Probability 0..1 of dropping at all. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Chance = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "1"))
	int32 MinCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drop", meta = (ClampMin = "1"))
	int32 MaxCount = 1;
};

/** One material requirement in a crafting recipe (FSnCraftRecipeRow). */
USTRUCT(BlueprintType)
struct FSnRecipeIngredient
{
	GENERATED_BODY()

	/** Required item (row name in the item data table). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FName ItemRowName = NAME_None;

	/** Required count per craft. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting", meta = (ClampMin = "1"))
	int32 Count = 1;
};

/**
 * One row in the crafting recipe data table (USnSettings::CraftRecipeDataTable).
 * The row name is the recipe's ID (e.g. "IronSword"). Pure static data:
 * crafting consumes the Materials and adds OutputCount of the output item,
 * server-executed and all-or-nothing. No per-recipe assets or classes - like
 * FSnItemRow, the whole system stays FStruct-friendly.
 */
USTRUCT(BlueprintType)
struct FSnCraftRecipeRow : public FTableRowBase
{
	GENERATED_BODY()

	/** The recipe's ID (the table row name), filled at runtime by the library
	 *  lookups (GetAllCraftRecipes / GetCraftRecipe). Not a table column - no
	 *  EditAnywhere, so it cannot be misconfigured - but Blueprints read it to
	 *  dispatch CraftItem. Same pattern as FSnItemActionInfo::RowName. */
	UPROPERTY(BlueprintReadOnly, Category = "Crafting")
	FName RowName = NAME_None;

	/** Recipe name shown in the crafting window list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FText DisplayName;

	/** Crafted item (row name in the item data table). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	FName OutputItemRowName = NAME_None;

	/** Items crafted per execution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting", meta = (ClampMin = "1"))
	int32 OutputCount = 1;

	/** Material requirements (each ingredient's Count per craft). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crafting")
	TArray<FSnRecipeIngredient> Materials;
};

/**
 * One row in the item action data table (USnSettings::ItemActionDataTable).
 * The row name is the action's ID (e.g. "Use"); the row decides when the
 * action shows in the right-click context menu (ItemType/Source filters)
 * and what runs when clicked (ActionClass). Pure static data - the runtime
 * execution instantiates UItemAction per click, so no state lives here.
 */
USTRUCT(BlueprintType)
struct FSnItemActionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Menu label (e.g. "Use"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FText DisplayName;

	/** Item tag required for the action to show; empty = any item. The action
	 *  shows when ANY of the item's tags matches (subtree match, so
	 *  "Item.Type.Equipment" also covers sub-types like
	 *  "Item.Type.Equipment.Weapon"). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FGameplayTag RequiredItemType;

	/** Slot context required for the action to show; empty = any source
	 *  (literal tags "Item.Source.Bag" / ".Equipment" / ".Storage", exact
	 *  match against the clicked slot's source). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FGameplayTag RequiredSource;

	/** When true the action only shows while a storage container is open
	 *  (transfers between bag and container). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	bool bRequiresStorageOpen = false;

	/** What runs when the menu entry is clicked. Hard reference on purpose
	 *  (same reasoning as FSnItemRow::EquipAbilities: the DataTable cell
	 *  editor doesn't support TSoftClassPtr selection). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	TSubclassOf<UItemAction> ActionClass;
};

/** What the context menu needs about one available action: the row name
 *  (dispatches execution back to ExecuteItemAction) and the label. */
USTRUCT(BlueprintType)
struct FSnItemActionInfo
{
	GENERATED_BODY()

	/** Row name in the item action data table (the action's ID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FName RowName = NAME_None;

	/** Menu label (copied from the action row). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FText DisplayName;
};

/** What an item action executes against: the clicked item plus where it
 *  sits. Execution resolves the right component method (equip/use/drop on
 *  the inventory, transfers on the storage) - the action never switches on
 *  item type, it just receives context. */
USTRUCT(BlueprintType)
struct FSnItemActionContext
{
	GENERATED_BODY()

	/** The item's row name (the item's ID). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FName ItemRowName = NAME_None;

	/** Where the clicked slot lives: "Item.Source.Bag" / ".Equipment" /
	 *  ".Storage" (literal tag, mirrors the slot's source). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FGameplayTag Source;

	/** Equipment grid cell of the clicked item; only meaningful when Source is
	 *  Item.Source.Equipment (remove-from-grid actions read it). INDEX_NONE
	 *  otherwise. Replaces the old per-slot tag: the grid has no slots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	int32 GridIndex = INDEX_NONE;

	/** Stack count of the clicked slot (drop/transfer actions move the
	 *  whole stack; right-click is the "do it now" gesture - no sub-menu). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta = (ClampMin = "1"))
	int32 Count = 1;

	/** Slot index of the clicked stack (bag/storage grids); the component
	 *  methods take it so the action targets the clicked group - not whatever
	 *  the row-name fallback finds (see USnInventoryComponent::RemoveItemAt).
	 *  INDEX_NONE when there's no bag slot (equipment grid cells use
	 *  GridIndex instead). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	int32 SourceIndex = INDEX_NONE;
};
