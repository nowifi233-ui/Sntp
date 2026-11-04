// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/WidgetController/SnAbilityWidgetController.h"
#include "AbilitySystem/SnAbilityCatalog.h"
#include "AbilitySystem/SnGameplayAbility.h"
#include "AbilitySystemComponent.h"
#include "Game/SnSettings.h"
#include "GameplayAbilitySpec.h"

void USnAbilityWidgetController::BroadcastInitialValues()
{
	// Pull-only: GetAbilities reads the ASC on demand, nothing to push.
}

TArray<FSnAbilityUIData> USnAbilityWidgetController::GetAbilities() const
{
	TArray<FSnAbilityUIData> Result;

	if (!AbilitySystemComponent)
	{
		return Result;
	}

	// Static metadata (icon / name / identity tag) lives in the catalog; only
	// abilities with a row surface in the UI.
	const USnAbilityCatalog* Catalog = USnSettings::Get()->GetAbilityCatalog();
	if (!Catalog)
	{
		return Result;
	}

	for (const FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		// Spec.Ability is always the class default object - the static config
		// (damage, range, cooldown GE, Description). No activation needed.
		USnGameplayAbility* Ability = Cast<USnGameplayAbility>(Spec.Ability);
		if (!Ability)
		{
			continue;
		}

		// The CDO's class is the blueprint/generated class, so blueprint
		// abilities resolve too. Abilities without a catalog row are skipped.
		const FSnAbilityInfo* Info = Catalog->FindInfoByClass(TSubclassOf<USnGameplayAbility>(Ability->GetClass()));
		if (!Info)
		{
			continue;
		}

		FSnAbilityUIData Data;
		Data.Tag = Info->Tag;
		Data.Icon = Info->Icon;
		Data.DisplayName = Info->DisplayName;
		Data.Level = Spec.Level;
		// Description generation is instance-state-free: safe on the CDO.
		Data.Description = Ability->GetDescription(Spec.Level);

		// Live cooldown state (0 = ready). UE 5.7 moved the query onto the
		// ability (UGameplayAbility::GetCooldownTimeRemaining(ActorInfo)): it
		// reads the ASC's active effects by the ability's cooldown tags - the
		// same OwningTagQuery family the attack cooldown ring uses - so it is
		// instance-state-free and safe on the CDO.
		if (const FGameplayAbilityActorInfo* ActorInfo = AbilitySystemComponent->AbilityActorInfo.Get())
		{
			Data.CooldownRemaining = Ability->GetCooldownTimeRemaining(ActorInfo);
		}

		Result.Add(Data);
	}
	return Result;
}
