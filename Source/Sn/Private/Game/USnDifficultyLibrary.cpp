// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/USnDifficultyLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "GameplayEffect.h"
#include "GameplayEffectComponents/TargetTagsGameplayEffectComponent.h"
#include "ScalableFloat.h"

const FSnDifficultyRow* USnDifficultyLibrary::FindDifficultyRow(FName DifficultyRowName)
{
	if (DifficultyRowName.IsNone())
	{
		return nullptr;
	}
	UDataTable* Table = USnSettings::Get()->GetDifficultyDataTable();
	return Table ? Table->FindRow<FSnDifficultyRow>(DifficultyRowName, TEXT("FindDifficultyRow")) : nullptr;
}

TArray<FSnDifficultyRow> USnDifficultyLibrary::GetAllDifficultyRows()
{
	TArray<FSnDifficultyRow> Rows;
	UDataTable* Table = USnSettings::Get()->GetDifficultyDataTable();
	if (!Table)
	{
		return Rows;
	}

	// Table row order - the spawner menu lists tiers in authored order.
	// RowName is filled from the table key (the field is not a table column),
	// so blueprints get the tier ID to dispatch SetDifficulty (same pattern as
	// USnItemLibrary::GetAllCraftRecipes).
	Table->ForeachRow<FSnDifficultyRow>(TEXT("USnDifficultyLibrary::GetAllDifficultyRows"),
		[&Rows](const FName& Key, const FSnDifficultyRow& Row)
		{
			FSnDifficultyRow Copy = Row;
			Copy.RowName = Key;
			Rows.Add(Copy);
		});
	return Rows;
}

bool USnDifficultyLibrary::ApplyDifficultyToPlayer(UAbilitySystemComponent* ASC, const FSnDifficultyRow& Row)
{
	if (!ASC)
	{
		return false;
	}

	// Remove any previous difficulty effect first: the GameMode hook runs on
	// every RestartPlayer, so a respawn or re-entry must re-apply instead of
	// stacking another MultiplyCompound layer.
	RemoveDifficultyFromPlayer(ASC);

	// One Infinite GE holding all four multipliers (MultiplyCompound: applied
	// after AddBase modifiers, so equipment bonuses are multiplied too - the
	// engine equation is (Base + AddBase) * MultiplyCompound + AddFinal).
	// Health uses the MaxHealth factor so the current health ratio survives.
	UGameplayEffect* Effect = NewObject<UGameplayEffect>(GetTransientPackage(), TEXT("Difficulty"));
	Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;

	// Grant Tags to Target Actor sits on a component since 5.3 (the GE's own
	// InheritableOwnedTagsContainer is deprecated): the component writes the GE's
	// CachedGrantedTags, which is what the applied spec reads into the ASC's tag
	// map - and therefore what RemoveActiveEffectsWithGrantedTags matches on. Drop
	// this and RemoveDifficultyFromPlayer would quietly find nothing, stacking
	// another layer of multipliers on every respawn.
	UTargetTagsGameplayEffectComponent& TargetTags = Effect->FindOrAddComponent<UTargetTagsGameplayEffectComponent>();
	FInheritedTagContainer GrantedTagChanges;
	GrantedTagChanges.AddTag(SnGameplayTags::Difficulty_Applied);
	TargetTags.SetAndApplyTargetTagChanges(GrantedTagChanges);

	const auto AddMultiplier = [Effect](const FGameplayAttribute& Attribute, float Multiplier)
	{
		if (FMath::IsNearlyEqual(Multiplier, 1.f))
		{
			return; // No-op tier factor - nothing to modify.
		}
		FGameplayModifierInfo Modifier;
		Modifier.Attribute = Attribute;
		Modifier.ModifierOp = EGameplayModOp::Type::MultiplyCompound;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(Multiplier));
		Effect->Modifiers.Add(Modifier);
	};

	AddMultiplier(USnAttributeSet::GetMaxHealthAttribute(), Row.PlayerMaxHealthMult);
	AddMultiplier(USnAttributeSet::GetHealthAttribute(), Row.PlayerMaxHealthMult);
	AddMultiplier(USnAttributeSet::GetPhysicalDamageAttribute(), Row.PlayerAttackMult);
	AddMultiplier(USnAttributeSet::GetMoveSpeedAttribute(), Row.PlayerMoveSpeedMult);

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	const FGameplayEffectSpec Spec(Effect, Context, 1.f);
	ASC->ApplyGameplayEffectSpecToSelf(Spec);
	return true;
}

void USnDifficultyLibrary::RemoveDifficultyFromPlayer(UAbilitySystemComponent* ASC)
{
	if (!ASC)
	{
		return;
	}
	FGameplayTagContainer Tags;
	Tags.AddTag(SnGameplayTags::Difficulty_Applied);
	ASC->RemoveActiveEffectsWithGrantedTags(Tags);
}
