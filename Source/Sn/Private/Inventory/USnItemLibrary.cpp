// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/USnItemLibrary.h"

#include "AbilitySystem/SnAbilityCatalog.h"
#include "AbilitySystem/SnAttributeCatalog.h"
#include "AbilitySystem/SnGameplayAbility.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Engine/Texture2D.h"
#include "Inventory/UItemAction.h"
#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnStorageComponent.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffect.h"
#include "Game/SnSettings.h"

const FSnItemRow* USnItemLibrary::FindItemRow(FName ItemRowName)
{
	if (ItemRowName.IsNone())
	{
		return nullptr;
	}

	const UDataTable* ItemTable = USnSettings::Get()->GetItemDataTable();
	if (!ItemTable)
	{
		return nullptr;
	}

	return ItemTable->FindRow<FSnItemRow>(ItemRowName, TEXT("USnItemLibrary::FindItemRow"));
}

const FSnBondRow* USnItemLibrary::FindBondRow(FGameplayTag BondTag)
{
	if (!BondTag.IsValid())
	{
		return nullptr;
	}

	const UDataTable* BondTable = USnSettings::Get()->GetBondDataTable();
	if (!BondTable)
	{
		return nullptr;
	}

	return BondTable->FindRow<FSnBondRow>(BondTag.GetTagName(), TEXT("USnItemLibrary::FindBondRow"));
}

const FSnItemRow* USnItemLibrary::FindItemRowChecked(FName ItemRowName, bool& bFound)
{
	const FSnItemRow* Row = FindItemRow(ItemRowName);
	bFound = Row != nullptr;
	return Row;
}

TArray<FName> USnItemLibrary::GetAllItemRowNames()
{
	TArray<FName> RowNames;
	const UDataTable* ItemTable = USnSettings::Get()->GetItemDataTable();
	if (!ItemTable)
	{
		return RowNames;
	}

	// Table row order - the cheat panel lists items in authored order.
	ItemTable->ForeachRow<FSnItemRow>(TEXT("USnItemLibrary::GetAllItemRowNames"),
		[&RowNames](const FName& Key, const FSnItemRow& Row)
		{
			RowNames.Add(Key);
		});
	return RowNames;
}

FText USnItemLibrary::GetItemDisplayName(FName ItemRowName)
{
	// No configured display name (empty cell or unknown row) -> empty text,
	// so UI renders nothing instead of a fallback name.
	const FSnItemRow* Row = FindItemRow(ItemRowName);
	return Row ? Row->DisplayName : FText::GetEmpty();
}

FText USnItemLibrary::GetStackDisplayName(FName ItemRowName, int32 Count)
{
	const FText Name = GetItemDisplayName(ItemRowName);
	if (Count > 1)
	{
		return FText::Format(NSLOCTEXT("SnItemLibrary", "StackFormat", "{0} \xD7{1}"), Name, FText::AsNumber(Count));
	}
	return Name;
}

FText USnItemLibrary::GetItemDescription(FName ItemRowName)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return Row->Description;
	}
	return FText::GetEmpty();
}

TSoftObjectPtr<UTexture2D> USnItemLibrary::GetItemIcon(FName ItemRowName)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return Row->Icon;
	}
	return nullptr;
}

UTexture2D* USnItemLibrary::GetItemIconLoaded(FName ItemRowName)
{
	return GetItemIcon(ItemRowName).LoadSynchronous();
}

FGameplayTagContainer USnItemLibrary::GetItemTags(FName ItemRowName)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return Row->ItemTags;
	}
	return FGameplayTagContainer();
}

FGameplayTagContainer USnItemLibrary::GetItemBonds(FName ItemRowName)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return Row->BondTags;
	}
	return FGameplayTagContainer();
}

bool USnItemLibrary::HasItemTag(FName ItemRowName, const FGameplayTag& Tag)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return HasAnyMatchingTag(Row->ItemTags, Tag);
	}
	return false;
}

bool USnItemLibrary::HasAnyMatchingTag(const FGameplayTagContainer& Container, const FGameplayTag& Tag)
{
	for (const FGameplayTag& Candidate : Container)
	{
		if (Candidate.MatchesTag(Tag))
		{
			return true;
		}
	}
	return false;
}

int32 USnItemLibrary::GetItemLevel(FName ItemRowName)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return FMath::Max(0, Row->Level);
	}
	return 0;
}

ESnItemQuality USnItemLibrary::GetItemQuality(FName ItemRowName)
{
	const FSnItemRow* Row = FindItemRow(ItemRowName);
	return Row ? Row->Quality : ESnItemQuality::Common;
}

FSnQualityStyleRow USnItemLibrary::GetQualityStyle(ESnItemQuality Quality)
{
	// Built-in fallback keeps the UI usable before/without the style table:
	// white name, faint dark background.
	FSnQualityStyleRow Fallback;
	Fallback.NameColor = FSlateColor(FLinearColor::White);
	Fallback.BorderColor = FSlateColor(FLinearColor(0.12f, 0.12f, 0.12f, 0.4f));
	Fallback.Background = nullptr;

	const UDataTable* StyleTable = USnSettings::Get()->GetQualityStyleDataTable();
	if (!StyleTable)
	{
		return Fallback;
	}

	// Row name = the enum name (Common / Uncommon / ...) - same "row name =
	// ID" convention as the enemy/item tables, so no lookup column needed.
	static const FName RowNames[] = {
		TEXT("Common"),
		TEXT("Uncommon"),
		TEXT("Rare"),
		TEXT("Epic"),
		TEXT("Legendary"),
	};
	const int32 QualityIndex = FMath::Clamp(static_cast<int32>(Quality), 0, UE_ARRAY_COUNT(RowNames) - 1);
	const FSnQualityStyleRow* Style = StyleTable->FindRow<FSnQualityStyleRow>(RowNames[QualityIndex], TEXT("USnItemLibrary::GetQualityStyle"));
	if (!Style)
	{
		return Fallback;
	}
	return *Style;
}

int32 USnItemLibrary::GetItemStackSize(FName ItemRowName)
{
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return FMath::Max(1, Row->StackSize);
	}
	return 1;
}

int32 USnItemLibrary::GetSellPrice(FName ItemRowName)
{
	// 0 doubles as "not sellable": the shop lists what is > 0, the detail panel
	// hides the line, and the server rejects the sale (see ServerSellItem).
	if (const FSnItemRow* Row = FindItemRow(ItemRowName))
	{
		return FMath::Max(0, Row->SellPrice);
	}
	return 0;
}

bool USnItemLibrary::IsValidItem(FName ItemRowName)
{
	return FindItemRow(ItemRowName) != nullptr;
}

int32 USnItemLibrary::AddItemToStacks(TArray<FSnItemStack>& Stacks, FName ItemRowName, int32 Count)
{
	if (Count <= 0 || !IsValidItem(ItemRowName))
	{
		return 0;
	}

	const int32 StackSize = GetItemStackSize(ItemRowName);
	int32 Remaining = Count;

	// Fill same-ID stacks first (slots already holding the item, not at cap).
	for (FSnItemStack& Stack : Stacks)
	{
		if (Stack.ItemRowName != ItemRowName || Stack.Count <= 0)
		{
			continue;
		}
		const int32 Room = StackSize - Stack.Count;
		if (Room <= 0)
		{
			continue;
		}
		const int32 Added = FMath::Min(Remaining, Room);
		Stack.Count += Added;
		Remaining -= Added;
		if (Remaining <= 0)
		{
			break;
		}
	}

	// Then fill empty slots. Fixed-slot model: the array length is the
	// capacity; empty slots are FSnItemStack with ItemRowName == NAME_None.
	for (FSnItemStack& Stack : Stacks)
	{
		if (Remaining <= 0)
		{
			break;
		}
		if (!Stack.ItemRowName.IsNone())
		{
			continue;
		}
		Stack.ItemRowName = ItemRowName;
		Stack.Count = FMath::Min(Remaining, StackSize);
		Remaining -= Stack.Count;
	}

	return Count - Remaining;
}

bool USnItemLibrary::RemoveItemFromStacks(TArray<FSnItemStack>& Stacks, FName ItemRowName, int32 Count)
{
	if (Count <= 0 || CountItemInStacks(Stacks, ItemRowName) < Count)
	{
		return false;
	}

	int32 Remaining = Count;
	for (int32 Index = Stacks.Num() - 1; Index >= 0 && Remaining > 0; --Index)
	{
		if (Stacks[Index].ItemRowName != ItemRowName)
		{
			continue;
		}
		const int32 Taken = FMath::Min(Stacks[Index].Count, Remaining);
		Stacks[Index].Count -= Taken;
		Remaining -= Taken;
		if (Stacks[Index].Count <= 0)
		{
			// Fixed-slot model: the slot stays, it just becomes empty again.
			Stacks[Index].ItemRowName = NAME_None;
		}
	}

	return Remaining == 0;
}

bool USnItemLibrary::RemoveItemFromStackAt(TArray<FSnItemStack>& Stacks, int32 Index, int32 Count)
{
	if (Count <= 0 || !Stacks.IsValidIndex(Index))
	{
		return false;
	}

	FSnItemStack& Stack = Stacks[Index];
	if (Stack.ItemRowName.IsNone() || Stack.Count < Count)
	{
		return false;
	}

	Stack.Count -= Count;
	if (Stack.Count <= 0)
	{
		// Fixed-slot model: the slot stays, it just becomes empty again.
		Stack.ItemRowName = NAME_None;
	}
	return true;
}

int32 USnItemLibrary::CountItemInStacks(const TArray<FSnItemStack>& Stacks, FName ItemRowName)
{
	int32 Total = 0;
	for (const FSnItemStack& Stack : Stacks)
	{
		if (Stack.ItemRowName == ItemRowName)
		{
			Total += Stack.Count;
		}
	}
	return Total;
}

void USnItemLibrary::SortStacks(TArray<FSnItemStack>& Stacks)
{
	// Merge counts by row name, keeping first-seen order for a stable sort key.
	TMap<FName, int32> Counts;
	TArray<FName> Order;
	for (const FSnItemStack& Stack : Stacks)
	{
		if (Stack.ItemRowName.IsNone() || Stack.Count <= 0)
		{
			continue;
		}
		if (!Counts.Contains(Stack.ItemRowName))
		{
			Order.Add(Stack.ItemRowName);
		}
		Counts.FindOrAdd(Stack.ItemRowName) += Stack.Count;
	}

	Order.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });

	// Re-pack into full stacks, row-name order, at the front of the array.
	// Fixed-slot model: the length never changes - the trailing slots become
	// empty. Merging also frees up empty slots (the one-click-sort expectation).
	const int32 SlotCount = Stacks.Num();
	Stacks.Reset(SlotCount);
	for (const FName& ItemRowName : Order)
	{
		const int32 StackSize = FMath::Max(1, GetItemStackSize(ItemRowName));
		int32 Remaining = Counts[ItemRowName];
		while (Remaining > 0)
		{
			FSnItemStack NewStack;
			NewStack.ItemRowName = ItemRowName;
			NewStack.Count = FMath::Min(Remaining, StackSize);
			Stacks.Add(NewStack);
			Remaining -= NewStack.Count;
		}
	}
	while (Stacks.Num() < SlotCount)
	{
		Stacks.Add(FSnItemStack());
	}
}

void USnItemLibrary::GetEquipStatLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines)
{
	OutLines.Reset();

	const FSnItemRow* Row = FindItemRow(ItemRowName);
	if (!Row)
	{
		return;
	}

	AppendStatLines(Row->EquipModifiers, Level, OutLines);
}

void USnItemLibrary::GetUseStatLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines)
{
	OutLines.Reset();

	const FSnItemRow* Row = FindItemRow(ItemRowName);
	if (!Row)
	{
		return;
	}

	AppendStatLines(Row->UseModifiers, Level, OutLines);
}

FText USnItemLibrary::FormatEquipDescription(FName ItemRowName, int32 Level)
{
	const FSnItemRow* Row = FindItemRow(ItemRowName);
	if (!Row)
	{
		return FText::GetEmpty();
	}
	return FillDescriptionTemplate(Row->EquipDescription, Row->EquipModifiers, Level);
}

namespace
{
	/** Appends one "{name}：{description}" line for AbilityClass at Level - the
	 *  shared core of the equip/use ability description lines (an equip row
	 *  grants an array of abilities, a use row a single one). The name comes
	 *  from the ability catalog (class display name fallback); the text is the
	 *  ability's own GetDescription at Level, so level-scaled numbers always
	 *  match the granted spec. No-op when the class is null or the ability has
	 *  no text. */
	void AppendAbilityDescriptionLine(const USnAbilityCatalog* Catalog, const TSubclassOf<UGameplayAbility>& AbilityClass, int32 Level, TArray<FText>& OutLines)
	{
		if (!AbilityClass)
		{
			return;
		}

		USnGameplayAbility* Ability = GetMutableDefault<USnGameplayAbility>(AbilityClass.Get());
		if (!Ability)
		{
			return;
		}

		const FText Description = Ability->GetDescription(FMath::Max(0, Level));
		if (Description.IsEmpty())
		{
			return;
		}

		FText Name;
		const FSnAbilityInfo* Info = Catalog ? Catalog->FindInfoByClass(TSubclassOf<USnGameplayAbility>(AbilityClass)) : nullptr;
		if (Info && !Info->DisplayName.IsEmpty())
		{
			Name = Info->DisplayName;
		}
		else
		{
			// Fallback: the raw class name (no catalog entry). Strip the
			// blueprint "_C" suffix - meaningless outside the editor.
			FString ClassName = AbilityClass->GetName();
			ClassName.RemoveFromEnd(TEXT("_C"));
			Name = FText::FromString(ClassName);
		}

		OutLines.Add(FText::Format(FText::FromString(TEXT("{0}：{1}")), Name, Description));
	}
}

void USnItemLibrary::GetEquipAbilityDescriptionLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines)
{
	OutLines.Reset();

	const FSnItemRow* Row = FindItemRow(ItemRowName);
	if (!Row)
	{
		return;
	}

	const USnAbilityCatalog* Catalog = USnSettings::Get()->GetAbilityCatalog();

	// One line per granted ability - the same rendering core as the use
	// segment below. Level = the level GrantEquipAbilities gave the spec.
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : Row->EquipAbilities)
	{
		AppendAbilityDescriptionLine(Catalog, AbilityClass, Level, OutLines);
	}
}

void USnItemLibrary::GetUseAbilityDescriptionLines(FName ItemRowName, int32 Level, TArray<FText>& OutLines)
{
	OutLines.Reset();

	const FSnItemRow* Row = FindItemRow(ItemRowName);
	if (!Row)
	{
		return;
	}

	// Row->UseAbility is a single ability (the equip field is an array), so at
	// most one line. Level = the level the use ability was granted at:
	// ServerUseItem grants FMath::Max(1, Row->Level) - pass the effective
	// level (rune bonus included) when the use comes from a grid slot.
	const USnAbilityCatalog* Catalog = USnSettings::Get()->GetAbilityCatalog();
	AppendAbilityDescriptionLine(Catalog, Row->UseAbility, Level, OutLines);
}

FText USnItemLibrary::FormatUseDescription(FName ItemRowName, int32 Level)
{
	const FSnItemRow* Row = FindItemRow(ItemRowName);
	if (!Row)
	{
		return FText::GetEmpty();
	}
	return FillDescriptionTemplate(Row->UseDescription, Row->UseModifiers, Level);
}

FText USnItemLibrary::GetBondDisplayName(FGameplayTag BondTag)
{
	if (!BondTag.IsValid())
	{
		return FText::GetEmpty();
	}
	const FSnBondRow* BondRow = FindBondRow(BondTag);
	return BondRow ? BondRow->DisplayName : FText::GetEmpty();
}

int32 USnItemLibrary::GetBondMaxLevel(FGameplayTag BondTag)
{
	if (!BondTag.IsValid())
	{
		return 0;
	}
	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return 0;
	}
	// Tiers are ascending by Count, so the last tier's Count is the max
	// level (the piece count that maxes the bond) - NOT Tiers.Num(), which
	// is the tier count.
	return BondRow->Tiers.Num() > 0 ? BondRow->Tiers.Last().Count : 0;
}

void USnItemLibrary::GetBondTierDescriptions(FGameplayTag BondTag, TArray<FText>& OutDescriptions)
{
	OutDescriptions.Reset();

	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return;
	}

	for (const FSnBondTier& Tier : BondRow->Tiers)
	{
		OutDescriptions.Add(FormatBondTierDescription(Tier));
	}
}

void USnItemLibrary::GetBondTierStatLines(FGameplayTag BondTag, TArray<FText>& OutStatLines)
{
	OutStatLines.Reset();

	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return;
	}

	for (const FSnBondTier& Tier : BondRow->Tiers)
	{
		// One entry per tier, newline-joined, so the array aligns 1:1 with
		// GetBondTierDescriptions (same index = same tier).
		TArray<FText> Lines;
		AppendStatLines(Tier.Modifiers, /*Level=*/1, Lines);
		OutStatLines.Add(FText::Join(FText::FromString(TEXT("\n")), Lines));
	}
}

void USnItemLibrary::GetBondTierAbilityLines(FGameplayTag BondTag, TArray<FText>& OutAbilityLines)
{
	OutAbilityLines.Reset();

	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return;
	}

	// One entry per tier (the tier has a single ability, so the join mirrors
	// GetBondTierStatLines purely for shape symmetry): "AbilityName: text" via
	// the same rendering core as the items' equip/use ability lines, at Level 1
	// - the level RecomputeBonds grants tier abilities at, so the shown
	// numbers always match the granted spec.
	const USnAbilityCatalog* Catalog = USnSettings::Get()->GetAbilityCatalog();
	for (const FSnBondTier& Tier : BondRow->Tiers)
	{
		TArray<FText> Lines;
		AppendAbilityDescriptionLine(Catalog, Tier.EquipAbility, /*Level=*/1, Lines);
		OutAbilityLines.Add(FText::Join(FText::FromString(TEXT("\n")), Lines));
	}
}

void USnItemLibrary::GetBondTierCounts(FGameplayTag BondTag, TArray<int32>& OutCounts)
{
	OutCounts.Reset();

	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return;
	}

	for (const FSnBondTier& Tier : BondRow->Tiers)
	{
		OutCounts.Add(Tier.Count);
	}
}

FText USnItemLibrary::GetBondDescriptionByCount(FGameplayTag BondTag, int32 Count)
{
	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return FText::GetEmpty();
	}

	for (const FSnBondTier& Tier : BondRow->Tiers)
	{
		if (Tier.Count == Count)
		{
			return FormatBondTierDescription(Tier);
		}
	}
	return FText::GetEmpty();
}

FText USnItemLibrary::GetBondAbilityLineByCount(FGameplayTag BondTag, int32 Count)
{
	const FSnBondRow* BondRow = FindBondRow(BondTag);
	if (!BondRow)
	{
		return FText::GetEmpty();
	}

	for (const FSnBondTier& Tier : BondRow->Tiers)
	{
		if (Tier.Count == Count)
		{
			// One ability per tier, so at most one line - same rendering core
			// as GetBondTierAbilityLines (catalog name, GetDescription at
			// Level 1 = the grant level). Empty when the tier grants none.
			const USnAbilityCatalog* Catalog = USnSettings::Get()->GetAbilityCatalog();
			TArray<FText> Lines;
			AppendAbilityDescriptionLine(Catalog, Tier.EquipAbility, /*Level=*/1, Lines);
			return Lines.Num() > 0 ? Lines[0] : FText::GetEmpty();
		}
	}
	return FText::GetEmpty();
}

FText USnItemLibrary::FormatBondTierDescription(const FSnBondTier& Tier)
{
	// Same placeholder fill as the item descriptions: {Attribute.X}
	// placeholders get the tier's EFFECTIVE values (Level 1 - bond values
	// are used as authored, no level scaling). "(Count) " prefix carries the
	// tier's required piece count in the same line, so the UI renders each
	// tier as one self-contained description row.
	FText Filled = FillDescriptionTemplate(Tier.Description, Tier.Modifiers, /*Level=*/1);
	return FText::Format(FText::FromString(TEXT("({0}) {1}")), Tier.Count, Filled);
}

TSoftObjectPtr<UTexture2D> USnItemLibrary::GetBondIcon(FGameplayTag BondTag)
{
	if (!BondTag.IsValid())
	{
		return nullptr;
	}
	const FSnBondRow* BondRow = FindBondRow(BondTag);
	return BondRow ? BondRow->Icon : nullptr;
}

FText USnItemLibrary::FillDescriptionTemplate(const FText& Template, const TArray<FSnItemStatModifier>& Modifiers, int32 Level)
{
	if (Template.IsEmpty())
	{
		return FText::GetEmpty();
	}

	// Collect EFFECTIVE values keyed by attribute tag. Order-independent:
	// the template names values by {Attribute.X}, never by position.
	FFormatNamedArguments Args;
	for (const FSnItemStatModifier& Modifier : Modifiers)
	{
		// Template placeholder keys are the native "Attribute.X" tags
		// (registered in SnGameplayTags) - deliberately NOT the SetByCaller
		// key (SetByCaller.X); the two conventions are independent.
		const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(FString(TEXT("Attribute.")) + Modifier.Attribute.GetName()), /*ErrorIfNotFound=*/false);
		if (!Tag.IsValid())
		{
			continue;
		}
		Args.Add(Tag.ToString(), FText::AsNumber(GetEffectiveValue(Modifier, Level)));
	}

	if (Args.Num() == 0)
	{
		return Template; // No matching modifier: placeholder-less template passes through.
	}
	return FText::Format(Template, Args);
}

const FSnItemActionRow* USnItemLibrary::FindActionRow(FName ActionRowName)
{
	if (ActionRowName.IsNone())
	{
		return nullptr;
	}

	const UDataTable* ActionTable = USnSettings::Get()->GetItemActionDataTable();
	if (!ActionTable)
	{
		return nullptr;
	}

	return ActionTable->FindRow<FSnItemActionRow>(ActionRowName, TEXT("USnItemLibrary::FindActionRow"));
}

// --- Crafting recipes ---

const FSnCraftRecipeRow* USnItemLibrary::FindRecipeRow(FName RecipeRowName)
{
	if (RecipeRowName.IsNone())
	{
		return nullptr;
	}

	const UDataTable* RecipeTable = USnSettings::Get()->GetCraftRecipeDataTable();
	if (!RecipeTable)
	{
		return nullptr;
	}

	return RecipeTable->FindRow<FSnCraftRecipeRow>(RecipeRowName, TEXT("USnItemLibrary::FindRecipeRow"));
}

TArray<FSnCraftRecipeRow> USnItemLibrary::GetAllCraftRecipes()
{
	TArray<FSnCraftRecipeRow> Recipes;
	const UDataTable* RecipeTable = USnSettings::Get()->GetCraftRecipeDataTable();
	if (!RecipeTable)
	{
		return Recipes;
	}

	// Table row order - the crafting window lists recipes in authored order.
	// RowName is filled from the table key (the field is not a table column),
	// so blueprints get the recipe ID to dispatch CraftItem.
	RecipeTable->ForeachRow<FSnCraftRecipeRow>(TEXT("USnItemLibrary::GetAllCraftRecipes"),
		[&Recipes](const FName& Key, const FSnCraftRecipeRow& Row)
		{
			FSnCraftRecipeRow Copy = Row;
			Copy.RowName = Key;
			Recipes.Add(Copy);
		});
	return Recipes;
}

FSnCraftRecipeRow USnItemLibrary::GetCraftRecipe(FName RecipeRowName)
{
	const FSnCraftRecipeRow* Row = FindRecipeRow(RecipeRowName);
	if (!Row)
	{
		return FSnCraftRecipeRow();
	}
	FSnCraftRecipeRow Copy = *Row;
	Copy.RowName = RecipeRowName;
	return Copy;
}

int32 USnItemLibrary::GetAvailableCraftCount(const FSnCraftRecipeRow& Recipe, const USnInventoryComponent* Inventory)
{
	if (!Inventory || Recipe.Materials.Num() == 0)
	{
		return 0;
	}

	int32 Available = MAX_int32;
	for (const FSnRecipeIngredient& Ingredient : Recipe.Materials)
	{
		if (Ingredient.Count <= 0)
		{
			return 0; // Misconfigured row: treat as uncraftable.
		}
		Available = FMath::Min(Available, Inventory->GetItemCount(Ingredient.ItemRowName) / Ingredient.Count);
	}
	return Available;
}

void USnItemLibrary::GetAvailableItemActions(FGameplayTagContainer ItemTags, FGameplayTag SourceTag, bool bStorageOpen, TArray<FSnItemActionInfo>& OutActions)
{
	OutActions.Reset();

	const UDataTable* ActionTable = USnSettings::Get()->GetItemActionDataTable();
	if (!ActionTable)
	{
		return;
	}

	for (const FName& RowName : ActionTable->GetRowNames())
	{
		const FSnItemActionRow* Row = ActionTable->FindRow<FSnItemActionRow>(RowName, TEXT("USnItemLibrary::GetAvailableItemActions"));
		if (!Row)
		{
			continue;
		}

		// Empty filters match anything; ANY of the item's tags may match the
		// required tag (subtree: "Item.Type.Equipment" covers future sub-types),
		// Source matches exactly (flat Item.Source.* tags).
		if (Row->RequiredItemType.IsValid() && !HasAnyMatchingTag(ItemTags, Row->RequiredItemType))
		{
			continue;
		}

		if (Row->RequiredSource.IsValid() && SourceTag != Row->RequiredSource)
		{
			continue;
		}

		if (Row->bRequiresStorageOpen && !bStorageOpen)
		{
			continue;
		}

		// Misconfigured rows (no label or no executor) would render a dead
		// menu entry - skip them so a broken row silently hides instead of
		// showing a no-op item.
		if (Row->DisplayName.IsEmpty() || !Row->ActionClass)
		{
			continue;
		}

		FSnItemActionInfo Info;
		Info.RowName = RowName;
		Info.DisplayName = Row->DisplayName;
		OutActions.Add(Info);
	}
}

void USnItemLibrary::ExecuteItemAction(FName ActionRowName, USnInventoryComponent* Inventory, USnStorageComponent* Storage, const FSnItemActionContext& Context)
{
	const FSnItemActionRow* Row = FindActionRow(ActionRowName);
	if (!Row || !Row->ActionClass)
	{
		return;
	}

	// Stateless per-execution instance (same pattern as UInteractionAction).
	UItemAction* Action = NewObject<UItemAction>(GetTransientPackage(), Row->ActionClass);
	Action->Execute(Inventory, Storage, Context);
}

void USnItemLibrary::AppendStatLines(const TArray<FSnItemStatModifier>& Modifiers, int32 Level, TArray<FText>& OutLines)
{
	const USnAttributeCatalog* Catalog = USnSettings::Get()->GetAttributeCatalog();
	for (const FSnItemStatModifier& Modifier : Modifiers)
	{
		if (!Modifier.Attribute.IsValid())
		{
			continue;
		}

		// Display name + rich-text tag from the catalog. Attributes without a
		// configured display name have no meaningful line for players - skip
		// them entirely (no raw-attribute-name fallback in stat lines).
		FText AttributeName;
		FString TagName;
		if (Catalog)
		{
			if (const FSnAttributeInfo* Info = Catalog->FindInfoByAttribute(Modifier.Attribute))
			{
				AttributeName = Info->DisplayName;
				TagName = Info->Tag.ToString();
			}
		}
		if (AttributeName.IsEmpty())
		{
			continue;
		}

		// The FINAL value at this item's level - stat lines show the effective
		// value, never the raw per-level breakdown. Near-zero lines are noise.
		const float Effective = GetEffectiveValue(Modifier, Level);
		if (FMath::IsNearlyZero(Effective))
		{
			continue;
		}

		const FText ValueText = FText::AsNumber(FMath::Abs(Effective));
		const FText Sign = Effective >= 0.f ? FText::FromString(TEXT("+")) : FText::FromString(TEXT("-"));
		if (!TagName.IsEmpty())
		{
			OutLines.Add(FText::Format(NSLOCTEXT("SnItemLibrary", "StatLineTagged", "<{0}>{1} {2}{3}</>"),
				FText::FromString(TagName), AttributeName, Sign, ValueText));
		}
		else
		{
			OutLines.Add(FText::Format(NSLOCTEXT("SnItemLibrary", "StatLine", "{0} {1}{2}"), AttributeName, Sign, ValueText));
		}
	}
}

float USnItemLibrary::GetEffectiveValue(const FSnItemStatModifier& Modifier, int32 Level)
{
	// Level scaling is purely additive per level above 0 (levels start at 0 -
	// level 0 IS the base value); a unified formula
	// covers both equipment (ValuePerLevel != 0) and consumables (usually 0).
	return Modifier.Value + FMath::Max(0, Level) * Modifier.ValuePerLevel;
}

// --- Rune shapes ---

const FSnRuneShapeRow* USnItemLibrary::FindRuneShapeRow(FName RuneShapeID)
{
	if (RuneShapeID.IsNone())
	{
		return nullptr;
	}

	const UDataTable* ShapeTable = USnSettings::Get()->GetRuneDataTable();
	if (!ShapeTable)
	{
		return nullptr;
	}

	return ShapeTable->FindRow<FSnRuneShapeRow>(RuneShapeID, TEXT("USnItemLibrary::FindRuneShapeRow"));
}

void USnItemLibrary::RotateOffsets(const TArray<FIntPoint>& InOffsets, int32 RotationSteps, TArray<FIntPoint>& OutOffsets)
{
	OutOffsets.Reset(InOffsets.Num());

	// Screen convention (X = column +right, Y = row +down): rotating a cell
	// to the right of the anchor 90 degrees clockwise lands it BELOW the
	// anchor - (X, Y) -> (-Y, X) - the rotation a player sees. Normalized to
	// mod 4: four steps return to the authored orientation.
	const int32 Steps = ((RotationSteps % 4) + 4) % 4;
	for (FIntPoint Offset : InOffsets)
	{
		for (int32 Step = 0; Step < Steps; ++Step)
		{
			Offset = FIntPoint(-Offset.Y, Offset.X);
		}
		OutOffsets.Add(Offset);
	}
}

void USnItemLibrary::GetCoveredCells(const FSnGridItem& Item, int32 GridSize, TArray<int32>& OutGridIndices)
{
	OutGridIndices.Reset();
	if (!Item.IsValid() || GridSize <= 0)
	{
		return;
	}

	const FSnItemRow* ItemRow = FindItemRow(Item.ItemRowName);
	const FSnRuneShapeRow* Shape = ItemRow ? FindRuneShapeRow(ItemRow->RuneShapeID) : nullptr;
	if (!Shape || Shape->Offsets.Num() == 0)
	{
		return; // Not a rune (or misconfigured shape): boosts nothing.
	}

	TArray<FIntPoint> Rotated;
	RotateOffsets(Shape->Offsets, Item.RotationSteps, Rotated);

	const int32 AnchorRow = Item.GridIndex / GridSize;
	const int32 AnchorCol = Item.GridIndex % GridSize;
	for (const FIntPoint& Offset : Rotated)
	{
		const int32 Row = AnchorRow + Offset.Y;
		const int32 Col = AnchorCol + Offset.X;

		// Boundary culling: cells outside the grid simply don't exist - a
		// cross in a corner boosts only the cells that are on the board.
		if (Row < 0 || Row >= GridSize || Col < 0 || Col >= GridSize)
		{
			continue;
		}
		OutGridIndices.Add(Row * GridSize + Col);
	}
}

FGameplayTag USnItemLibrary::GetSetByCallerTag(const FGameplayAttribute& Attribute)
{
	if (!Attribute.IsValid())
	{
		return FGameplayTag();
	}
	// SetByCaller keys are "SetByCaller.<AttributeName>" - the project's
	// existing SetByCaller.Damage / SetByCaller.XP convention, and the only
	// tags the GE asset editor's SetByCaller tag picker offers (it filters to
	// the SetByCaller root; "Attribute.X" tags can't be selected there). The
	// tags are registered in SnGameplayTags (or Project Settings > Gameplay
	// Tags); an unregistered attribute returns an invalid tag and the caller
	// logs a warning instead of silently applying 0. FGameplayAttribute::
	// GetName() returns the bare name as FString.
	return FGameplayTag::RequestGameplayTag(FName(FString(TEXT("SetByCaller.")) + Attribute.GetName()), /*ErrorIfNotFound=*/false);
}

FActiveGameplayEffectHandle USnItemLibrary::ApplyItemModifiers(
	UAbilitySystemComponent* ASC,
	const TSubclassOf<UGameplayEffect>& EffectClass,
	const TArray<FSnItemStatModifier>& Modifiers,
	int32 Level,
	float DurationSeconds)
{
	if (!ASC || !EffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	const float SpecLevel = FMath::Max(1.f, (float)Level);
	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, SpecLevel, Context);
	if (!Spec.IsValid())
	{
		return FActiveGameplayEffectHandle();
	}

	// Feed the row's modifiers as SetByCaller values, keyed by attribute tag.
	// The generic GE asset carries one SetByCaller modifier per common
	// attribute; entries without a registered tag are skipped with a warning
	// (config error: the modifier would otherwise silently apply as 0).
	for (const FSnItemStatModifier& Modifier : Modifiers)
	{
		const float Effective = GetEffectiveValue(Modifier, Level);
		if (FMath::IsNearlyZero(Effective))
		{
			continue;
		}
		const FGameplayTag Tag = GetSetByCallerTag(Modifier.Attribute);
		if (!Tag.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("ApplyItemModifiers: attribute '%s' has no registered 'SetByCaller.*' tag; modifier skipped"),
				*Modifier.Attribute.GetName());
			continue;
		}
		Spec.Data->SetSetByCallerMagnitude(Tag, Effective);
	}

	// The generic GE asset carries one SetByCaller modifier per common
	// attribute, but a row only sets the ones it references (the loop above).
	// Applying with unset SetByCaller magnitudes makes the engine log
	// "GetMagnitude called ... when magnitude had not yet been set by caller"
	// once per unused modifier (apply-time pre-evaluation, warn-on-fail
	// defaults to true) and evaluate it as 0 anyway. Fill the gaps with
	// explicit zeros: on an Additive modifier 0 is a true no-op, so the
	// semantics stay "this row does not touch that stat". Non-Additive
	// modifiers are deliberately left alone - zeroing a Multiply/Override
	// would silently corrupt the stat, and the engine error keeps flagging
	// such a config instead.
	// The asset's modifier list lives on the shared Def and is only read
	// here; the fill goes into per-spec SetByCallerTagMagnitudes, so one
	// application never affects another row's spec.
	const UGameplayEffect* EffectDef = Spec.Data->Def;
	for (const FGameplayModifierInfo& Modifier : EffectDef->Modifiers)
	{
		if (Modifier.ModifierMagnitude.GetMagnitudeCalculationType() != EGameplayEffectMagnitudeCalculation::SetByCaller)
		{
			continue;
		}
		const FGameplayTag& DataTag = Modifier.ModifierMagnitude.GetSetByCallerFloat().DataTag;
		if (!DataTag.IsValid() || Spec.Data->SetByCallerTagMagnitudes.Contains(DataTag))
		{
			continue;
		}
		if (Modifier.ModifierOp == EGameplayModOp::Additive)
		{
			Spec.Data->SetSetByCallerMagnitude(DataTag, 0.f);
		}
	}

	// Per-item duration (buff GE): override the asset's duration. Only takes
	// effect on duration-policy GEs; Instant GEs ignore it.
	if (DurationSeconds > 0.f)
	{
		Spec.Data->SetDuration(DurationSeconds, /*bLockNewDuration=*/true);
	}

	return ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void USnItemLibrary::GrantEquipAbilities(UAbilitySystemComponent* ASC, const TArray<TSubclassOf<UGameplayAbility>>& EquipAbilities, int32 Level, TArray<FGameplayAbilitySpecHandle>& OutHandles)
{
	if (!ASC)
	{
		return;
	}

	const int32 SpecLevel = FMath::Max(0, Level);

	// GA growth: abilities are given at the item's level; the ability class
	// reads level-dependant magnitudes from the spec.
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : EquipAbilities)
	{
		if (!AbilityClass)
		{
			continue;
		}
		const FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, SpecLevel));
		if (!Handle.IsValid())
		{
			continue;
		}
		OutHandles.Add(Handle);

		// Grant + activate in one, owned by the equip flow (the equip window IS
		// the passive's window - enchantment / sustain / summon shells set
		// bActivateOnEquip). Only THIS flow reads the flag: a grant from
		// anywhere else never auto-activates (e.g. a timed trigger pre-granting
		// its child releases it per pulse instead). TryActivateAbility in the
		// same stack is safe - the spec exists and is not active yet. A failed
		// activation on a fresh equip grant means the class blocks itself
		// (CanActivateAbility) - worth seeing in the log, not failing silently.
		const USnGameplayAbility* AbilityCDO = Cast<USnGameplayAbility>(AbilityClass->GetDefaultObject());
		if (AbilityCDO && AbilityCDO->bActivateOnEquip
			&& !ASC->TryActivateAbility(Handle, /*bAllowRemoteActivation=*/false))
		{
			UE_LOG(LogTemp, Warning, TEXT("USnItemLibrary: equip ability %s failed to activate on grant"), *AbilityClass->GetName());
		}
	}
}
