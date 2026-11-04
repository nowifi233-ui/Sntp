// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_Enchantment.h"

#include "AbilitySystem/SnGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Combat/USnCombatLibrary.h"

USnGA_Enchantment::USnGA_Enchantment()
{
	// One instance per actor, activated once on equip and kept alive: the
	// event listener runs the on-hit logic directly, so a hit never pays the
	// per-activation cost (ability state machine + instance churn).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// The equip flow grants AND activates this (bActivateOnEquip is read by
	// USnItemLibrary::GrantEquipAbilities right after the grant); unequipping
	// revokes the ability, which cancels this activation and unbinds the
	// listener in EndAbility. No OnAvatarSet self-activation: the equip flow
	// owns activation.
	bActivateOnEquip = true;
}

void USnGA_Enchantment::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Listen for the attack-hit event (fired by the attack GA per target) for
	// as long as this ability stays active. The tag-container delegate
	// dispatch (HandleGameplayEvent) matches Event.AttackHit and runs the
	// append logic directly - no per-hit ability activation cost.
	EventFilter = FGameplayTagContainer();
	EventFilter.AddTag(SnGameplayTags::Event_AttackHit);
	EventDelegateHandle = ASC->AddGameplayEventTagContainerDelegate(EventFilter, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &USnGA_Enchantment::HandleAttackEvent));
}

void USnGA_Enchantment::HandleAttackEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (!Payload || EventTag != SnGameplayTags::Event_AttackHit)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !DamageEffectClass)
	{
		return;
	}

	// The hit target (the attack GA sets Payload.Target). The struct field is
	// const-typed, but the actor itself is not - safe to strip for the ASC
	// lookup (the blueprint library only has the non-const overload).
	AActor* Target = const_cast<AActor*>(Payload->Target.Get());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}

	// Appended hit: the source's damage of this element x the level-scaled
	// multiplier (the shared damage-family formula - level 0, the base
	// equipment level, applies the Multiplier alone and each level adds the
	// configured growth as a flat addition, not a compound factor), as an
	// independent application - it goes through the full ExecCalc pipeline
	// (element resistance, crit, toughness, defense) and renders its own
	// damage number.
	FSnDamageResult Damage = USnCombatLibrary::CalculateDamage(SourceASC, DamageType);
	Damage.Damage = USnGA_DamageSkill::ScaleDamageByLevel(Damage.Damage, Multiplier, PerLevelGrowth, GetAbilityLevel());
	if (Damage.Damage <= 0.f)
	{
		return;
	}

	USnCombatLibrary::ApplyDamageToTarget(SourceASC, DamageEffectClass, Avatar, TargetASC, Damage, GetAbilityLevel());
}

void USnGA_Enchantment::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Unbind the listener when the ability ends (cancelled on unequip, level
	// travel, or explicit cancellation). Handle-based removal matched by the
	// same tag container - idempotent, safe when never activated.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(EventFilter, EventDelegateHandle);
	}
	EventDelegateHandle.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FText USnGA_Enchantment::GetDescription_Implementation(int32 Level)
{
	// The shared element clause renders the effective percentage at Level
	// (the same growth expression ScaleDamageByLevel uses - one formula, one
	// text builder), so the panel number always matches the gameplay number.
	return FText::Format(
		FText::FromString(TEXT("攻击命中时，额外造成 {0}")),
		USnGA_DamageSkill::BuildDamageClause(Level, Multiplier, PerLevelGrowth, DamageType));
}
