// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_Ignite.h"

#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Combat/USnCombatLibrary.h"
#include "GameplayEffect.h"

USnGA_Ignite::USnGA_Ignite()
{
	// One instance per actor, activated once on tier grant and kept alive: the
	// event listener rolls and applies the ignite directly, so a hit never
	// pays the per-activation cost (same shape as USnGA_Enchantment).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// The bond flow grants AND activates this (bActivateOnEquip is read by
	// USnItemLibrary::GrantEquipAbilities right after the grant); losing the
	// tier revokes the ability, which cancels this activation and unbinds the
	// listener in EndAbility. No OnAvatarSet self-activation: the bond flow
	// owns activation.
	bActivateOnEquip = true;
}

void USnGA_Ignite::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Listen for the attack-hit event (fired by the shared attack core per
	// damaged target) for as long as this ability stays active. The tag-container
	// delegate dispatch matches Event.AttackHit and runs the ignite logic
	// directly - no per-hit ability activation cost.
	EventFilter = FGameplayTagContainer();
	EventFilter.AddTag(SnGameplayTags::Event_AttackHit);
	EventDelegateHandle = ASC->AddGameplayEventTagContainerDelegate(EventFilter, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &USnGA_Ignite::HandleAttackEvent));
}

void USnGA_Ignite::HandleAttackEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (!Payload || EventTag != SnGameplayTags::Event_AttackHit)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !IgniteEffectClass || TickDamage <= 0.f || BurnDurationSeconds <= 0.f)
	{
		return;
	}

	// Element filter: only fire hits ignite (the attack core now puts the
	// hit's element on the event's effect context - every project context is
	// an FSnGameplayEffectContext). An enchantment-stacked fire hit carries the
	// fire element too, so its appended damage can ignite as well.
	const FSnGameplayEffectContext* HitContext = static_cast<const FSnGameplayEffectContext*>(Payload->ContextHandle.Get());
	if (!HitContext || HitContext->GetDamageType() != EDamageType::Fire)
	{
		return;
	}

	// The hit target (the attack core sets Payload.Target). The struct field
	// is const-typed, but the actor itself is not - safe to strip for the ASC
	// lookup (the blueprint library only has the non-const overload).
	AActor* Target = const_cast<AActor*>(Payload->Target.Get());
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!TargetASC)
	{
		return;
	}

	// One burn at a time - no refresh: a target already burning (the burning
	// GE grants Status.Burning) is skipped, so the DoT always runs its course
	// before a later fire hit can ignite again. Cheap guard: the tag rides on
	// the target's ASC (active-effect granted tags are replicated to clients).
	if (TargetASC->HasMatchingGameplayTag(SnGameplayTags::Status_Burning))
	{
		return;
	}

	if (FMath::FRand() >= IgniteChance)
	{
		return;
	}

	// The burn spec: Damage = -TickDamage under the element's SetByCaller key
	// (read by USnExecCalc_DamageDot on every period), Duration under
	// SetByCaller.BurnDuration (the GE's Duration magnitude picks it). Built
	// from the attacker's ASC so the context carries the attacker as
	// instigator - every burn tick resolves through the target's damage chain
	// with kill credit intact (same shape as USnCombatLibrary::MakeDamageSpec).
	// Level 1: bond values are authored as-is, no level scaling.
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Avatar, Avatar);
	Context.AddSourceObject(Avatar);
	if (FSnGameplayEffectContext* SnContext = static_cast<FSnGameplayEffectContext*>(Context.Get()))
	{
		SnContext->SetDamageType(EDamageType::Fire);
	}

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(IgniteEffectClass, /*AbilityLevel=*/1, Context);
	if (!Spec.IsValid())
	{
		return;
	}

	Spec.Data->SetSetByCallerMagnitude(USnCombatLibrary::GetDamageSetByCallerKey(EDamageType::Fire), -TickDamage);
	Spec.Data->SetSetByCallerMagnitude(SnGameplayTags::SetByCaller_BurnDuration, BurnDurationSeconds);
	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
}

void USnGA_Ignite::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Unbind the listener when the ability ends (cancelled on tier loss, level
	// travel, or explicit cancellation). Handle-based removal matched by the
	// same tag container - idempotent, safe when never activated.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(EventFilter, EventDelegateHandle);
	}
	EventDelegateHandle.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FText USnGA_Ignite::GetDescription_Implementation(int32 Level)
{
	// Per-tier values are authored on the CDO (no level scaling), so the
	// description shows the chance, the per-tick damage and the duration
	// as-is. Convention: GE_Burn ticks once per second (the asset's Period
	// matches this wording - set it to 1 in the effect blueprint).
	const int32 Percent = FMath::RoundToInt(IgniteChance * 100.f);
	return FText::Format(
		FText::FromString(TEXT("造成火焰伤害时，有 {0}% 概率点燃敌人：每秒受到 {1} 点<{2}>{3}</>伤害，持续 {4} 秒")),
		Percent,
		FMath::RoundToInt(TickDamage),
		FText::FromName(USnCombatLibrary::GetDamageTypeStyleTag(EDamageType::Fire)),
		USnCombatLibrary::GetDamageTypeDisplayName(EDamageType::Fire),
		FMath::RoundToInt(BurnDurationSeconds));
}
