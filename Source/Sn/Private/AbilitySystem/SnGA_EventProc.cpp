// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_EventProc.h"

#include "AbilitySystem/SnAttributeCatalog.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Combat/USnCombatLibrary.h"
#include "Game/SnSettings.h"
#include "Inventory/USnItemLibrary.h"

namespace
{
	/** Is this one of the damage events - the two whose effect context carries a
	 *  real element, and therefore the only ones the element filter applies to?
	 *  The resource-gained events come from a heal / shield / mana effect whose
	 *  context holds whatever the default damage type is, so filtering on it
	 *  would be meaningless (and would silently drop them). Adding a damage event
	 *  to the project means adding it here too, next to its tag doc. */
	bool IsDamageEvent(const FGameplayTag& Tag)
	{
		return Tag.MatchesTagExact(SnGameplayTags::Event_AttackHit)
			|| Tag.MatchesTagExact(SnGameplayTags::Event_DamageTaken);
	}

	/** Human word for the trigger, derived from the filter: the known event tags
	 *  read naturally in a tooltip ("每 3 次命中：…"), anything else falls back to
	 *  the neutral "触发" - a future event tag needs no change here, and
	 *  blueprints can override GetDescription for flavored text.
	 *
	 *  With an element filter on a damage event the word becomes the whole
	 *  sentence ("造成火焰伤害" / "受到火焰伤害"), element names colored through the
	 *  same rich-text style rows the damage numbers use. A container with several
	 *  tags stays neutral: "触发" is the only word true for all of them. */
	FString TriggerWordFor(const FGameplayTagContainer& Filter, const TArray<EDamageType>& DamageTypes)
	{
		if (Filter.Num() != 1)
		{
			return TEXT("触发");
		}

		FString Word;
		FString DamageVerb;
		if (Filter.HasTagExact(SnGameplayTags::Event_AttackHit))
		{
			Word = TEXT("命中");
			DamageVerb = TEXT("造成");
		}
		else if (Filter.HasTagExact(SnGameplayTags::Event_DamageTaken))
		{
			Word = TEXT("受到伤害");
			DamageVerb = TEXT("受到");
		}
		else if (Filter.HasTagExact(SnGameplayTags::Event_Kill))
		{
			Word = TEXT("击杀敌人");
		}
		else if (Filter.HasTagExact(SnGameplayTags::Event_AttributeGained_Health))
		{
			Word = TEXT("回复生命");
		}
		else if (Filter.HasTagExact(SnGameplayTags::Event_AttributeGained_Shield))
		{
			Word = TEXT("获得护盾");
		}
		else if (Filter.HasTagExact(SnGameplayTags::Event_AttributeGained_MP))
		{
			Word = TEXT("回复法力");
		}
		else
		{
			return TEXT("触发");
		}

		if (DamageVerb.IsEmpty() || DamageTypes.IsEmpty())
		{
			return Word;
		}

		TArray<FString> ElementNames;
		ElementNames.Reserve(DamageTypes.Num());
		for (const EDamageType DamageType : DamageTypes)
		{
			ElementNames.Add(FString::Printf(TEXT("<%s>%s</>"),
				*USnCombatLibrary::GetDamageTypeStyleTag(DamageType).ToString(),
				*USnCombatLibrary::GetDamageTypeDisplayName(DamageType).ToString()));
		}
		return FString::Printf(TEXT("%s%s伤害"), *DamageVerb, *FString::Join(ElementNames, TEXT("/")));
	}

	/** Who the payload lands on, as a word for the description - and only when
	 *  that is somebody other than the listener. The word comes from the (mode,
	 *  event) pair, because one mode names different people on different events:
	 *  EventInstigator is the attacker on Event.DamageTaken but the listener
	 *  itself on Event.AttackHit. Pairs whose actor is always the listener get no
	 *  word - there is nothing to name, and naming it would be a lie. */
	FString TargetWordFor(const FGameplayTagContainer& Filter, ESnEventTargetMode Mode)
	{
		// Several events at once: the word would be true for some of them only.
		if (Filter.Num() != 1 || Mode == ESnEventTargetMode::Self)
		{
			return FString();
		}

		if (Mode == ESnEventTargetMode::EventInstigator)
		{
			if (Filter.HasTagExact(SnGameplayTags::Event_DamageTaken))
			{
				return TEXT("攻击者");
			}
			if (Filter.HasTagExact(SnGameplayTags::Event_AttributeGained_Health)
				|| Filter.HasTagExact(SnGameplayTags::Event_AttributeGained_Shield)
				|| Filter.HasTagExact(SnGameplayTags::Event_AttributeGained_MP))
			{
				// The effect's source: the healer on a heal somebody else cast.
				return TEXT("事件来源");
			}
			return FString();
		}

		// Only Event.AttackHit is raised at ANOTHER actor's ASC with that actor
		// as the target; the rest carry the listener in Payload.Target.
		return Filter.HasTagExact(SnGameplayTags::Event_AttackHit) ? TEXT("目标") : FString();
	}

	/** "10" / "20%（按触发数值）" / "10 + 20%（按触发数值，每级 +2%）" - the
	 *  level-scaled value text both payloads share. The share shows its value at
	 *  THIS level, the same way the flat part does, with the growth spelled out
	 *  next to it. Empty when the payload would do nothing at this level. */
	FString PayloadValueText(float Flat, float Ratio, float RatioPerLevel)
	{
		FString Text;
		if (!FMath::IsNearlyZero(Flat))
		{
			Text = FString::Printf(TEXT("%g"), Flat);
		}
		if (Ratio > 0.f)
		{
			FString RatioText = FString::Printf(TEXT("%g%%（按触发数值"), Ratio * 100.f);
			if (RatioPerLevel > 0.f)
			{
				RatioText += FString::Printf(TEXT("，每级 +%g%%"), RatioPerLevel * 100.f);
			}
			RatioText += TEXT("）");
			Text = Text.IsEmpty() ? RatioText : FString::Printf(TEXT("%s + %s"), *Text, *RatioText);
		}
		return Text;
	}

	/** Display name for the attribute from the attribute catalog (the same text
	 *  the item panel shows for equip rows), falling back to the raw attribute
	 *  name when the catalog has no entry or Sn Settings has none. */
	FString AttributeDisplayName(const FGameplayAttribute& Attribute)
	{
		if (const USnSettings* Settings = USnSettings::Get())
		{
			if (const USnAttributeCatalog* Catalog = Settings->GetAttributeCatalog())
			{
				if (const FSnAttributeInfo* Info = Catalog->FindInfoByAttribute(Attribute))
				{
					if (!Info->DisplayName.IsEmpty())
					{
						return Info->DisplayName.ToString();
					}
				}
			}
		}
		return Attribute.GetName();
	}
}

USnGA_EventProc::USnGA_EventProc()
{
	// One instance per actor, activated once on equip and kept alive: the count
	// lives on the instance and a trigger never pays the per-activation cost.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Server-authoritative: every trigger source only fires where the effect
	// executes, the counter is deliberately not replicated (see the class doc),
	// and the payload's own effects / cues replicate by themselves.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// The equip flow grants AND activates this (bActivateOnEquip is read by
	// USnItemLibrary::GrantEquipAbilities right after the grant); unequipping
	// revokes the ability, which cancels this activation and unbinds the
	// listener in EndAbility. No OnAvatarSet self-activation: the equip flow
	// owns activation.
	bActivateOnEquip = true;

	// Default trigger: the primary-hit event, so the common "on hit" proc is
	// one field away; a heal / shield listener swaps the container in the BP.
	TriggerEventTags.AddTag(SnGameplayTags::Event_AttackHit);
}

void USnGA_EventProc::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Payload switches default to OFF (the details panel shows three switches and
	// nothing else on a fresh asset), so an asset configured BEFORE they existed
	// has its classes assigned with the boxes still unticked - the payload would
	// silently never fire, and "the item does nothing" would be the only symptom.
	// Activation runs once per equip, so the warning is cheap and never spams.
	TArray<FString> DisabledPayloads;
	if (!bEnableEffectPayload && EffectClass)
	{
		DisabledPayloads.Add(TEXT("Effect"));
	}
	if (!bEnableDamagePayload && DamageEffectClass)
	{
		DisabledPayloads.Add(TEXT("Damage"));
	}
	if (!bEnableAbilityPayload && TriggeredAbilityClass)
	{
		DisabledPayloads.Add(TEXT("TriggeredAbility"));
	}
	if (DisabledPayloads.Num() > 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnGA_EventProc: %s has a configured payload behind a switched-off enable box (%s) - tick the box or clear the class"),
			*GetClass()->GetName(), *FString::Join(DisabledPayloads, TEXT(", ")));
	}

	// Nothing to listen for: the asset is misconfigured. End quietly but leave a
	// trace - a passive that silently does nothing is invisible until someone
	// wonders why the item has no effect.
	if (TriggerEventTags.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("USnGA_EventProc: %s has no TriggerEventTags - ending"), *GetClass()->GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Listen for the trigger events for as long as this ability stays active.
	// The tag-container delegate dispatch (HandleGameplayEvent) runs the counter
	// directly - no per-trigger ability activation cost.
	EventFilter = TriggerEventTags;
	EventDelegateHandle = ASC->AddGameplayEventTagContainerDelegate(EventFilter, FGameplayEventTagMulticastDelegate::FDelegate::CreateUObject(this, &USnGA_EventProc::HandleTriggerEvent));

	// Pre-grant the child once for the whole active window (the timed-trigger
	// pattern): payouts are then plain TryActivateAbility calls with no
	// per-payout grant / remove churn. EndAbility revokes it - see the paired
	// removal there. A switched-off ability payload is never granted at all (no
	// point giving a spec that can never be released).
	if (bEnableAbilityPayload && TriggeredAbilityClass && !TriggeredAbilityHandle.IsValid())
	{
		TriggeredAbilityHandle = ASC->GiveAbility(FGameplayAbilitySpec(TriggeredAbilityClass, GetAbilityLevel()));
	}
}

void USnGA_EventProc::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Paired with the pre-grant in ActivateAbility: the child spec only exists
	// inside this ability's active window. Every exit path lands here (spec
	// removal on unequip, cancel, level travel - the engine ends the active
	// instance), so this is the single point that keeps the model leak-free.
	if (TriggeredAbilityHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			ASC->ClearAbility(TriggeredAbilityHandle);
		}
		TriggeredAbilityHandle = FGameplayAbilitySpecHandle();
	}

	// Unbind the listener (cancelled on unequip, level travel, or explicit
	// cancellation). Handle-based removal matched by the same tag container -
	// idempotent, safe when never activated.
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		ActorInfo->AbilitySystemComponent->RemoveGameplayEventTagContainerDelegate(EventFilter, EventDelegateHandle);
	}
	EventDelegateHandle.Reset();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void USnGA_EventProc::HandleTriggerEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	// The tag-container delegate already filtered by TriggerEventTags; the tag
	// itself is informational here.

	// Re-entrancy latch (see the class doc): this handler runs synchronously
	// inside the effect application that raised the event, so a payout that
	// raises a trigger of its own, or of a proc feeding this one, would recurse
	// until the stack dies. Nested triggers are dropped; sequential ones - the
	// normal case - are untouched.
	if (bExecutingPayload)
	{
		return;
	}

	// The effect context rides along on every event this project raises (see
	// USnAttributeSet::SendGameplayEvent); it is where the hit's element and the
	// riposte flag live.
	const FSnGameplayEffectContext* EventContext = Payload ? static_cast<const FSnGameplayEffectContext*>(Payload->ContextHandle.Get()) : nullptr;

	// Element filter, damage events only. The hit's element rides on the effect
	// context (the same read the ignite passive does). Checked BEFORE the counter
	// on purpose: a physical hit must not advance "every 3 fire hits".
	if (!TriggerDamageTypes.IsEmpty() && IsDamageEvent(EventTag))
	{
		if (!EventContext || !TriggerDamageTypes.Contains(EventContext->GetDamageType()))
		{
			return;
		}
	}

	if (++TriggerCount < FMath::Max(1, EventsPerTrigger))
	{
		return;
	}
	TriggerCount = 0;

	// EventMagnitude is the trigger's own number - the damage dealt on a hit,
	// the applied delta on a resource gain. A trigger carrying none (a miss, a
	// zero-value gained event) leaves the ratio halves at 0: a payout with a flat
	// value still lands, a pure-ratio one is skipped by the payload itself.
	const float EventMagnitude = Payload ? Payload->EventMagnitude : 0.f;
	AActor* TargetActor = ResolvePayloadTargetActor(Payload);

	bExecutingPayload = true;
	ApplyEffectPayload(EventMagnitude, TargetActor);

	// Cross-instance loop, closed here: damage that ANSWERS a hit is a riposte,
	// and a riposte never raises another one - otherwise two units wearing this
	// same passive would bounce damage back and forth until the stack dies (the
	// latch above is per instance and cannot see the other unit's copy). Only
	// the damage payload is gated: the effect payload above still pays out on a
	// riposte hit, because taking one is still taking a hit.
	const bool bAnswersAHit = EventTag.MatchesTagExact(SnGameplayTags::Event_DamageTaken)
		&& EventContext && EventContext->IsReflected();
	if (!bAnswersAHit)
	{
		ApplyDamagePayload(EventTag, EventMagnitude, EventContext, TargetActor);
	}

	ReleaseTriggeredAbility();
	bExecutingPayload = false;
}

AActor* USnGA_EventProc::ResolvePayloadTargetActor(const FGameplayEventData* Payload) const
{
	switch (PayloadTargetMode)
	{
	case ESnEventTargetMode::EventInstigator:
		// The weak pointer is const-typed while the actor is not - the ASC
		// lookup below only has the non-const overload, so the const comes off
		// here (same strip the enchantment passive does).
		return Payload ? const_cast<AActor*>(Payload->Instigator.Get()) : nullptr;

	case ESnEventTargetMode::EventTarget:
		return Payload ? const_cast<AActor*>(Payload->Target.Get()) : nullptr;

	default:
		return GetAvatarActorFromActorInfo();
	}
}

void USnGA_EventProc::ApplyEffectPayload(float EventMagnitude, AActor* TargetActor)
{
	// The enable switch is the payload's master gate (the fields are hidden
	// behind it in the editor, but the values survive, so this is what actually
	// decides).
	if (!bEnableEffectPayload || !EffectClass || !EffectAttribute.IsValid() || !TargetActor)
	{
		return;
	}

	// The resolved actor may be somebody else (a debuff on the attacker, a buff
	// on the healer); actors without an ASC - destructibles - are skipped.
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	// Payout value: the flat level-scaled part plus a share of whatever raised
	// the trigger - 20% of the damage dealt, of the damage taken and of the
	// amount healed are the same expression over the event's magnitude. Both
	// halves grow with the effective item level, independently: the share here,
	// the flat value through the item path below (ValuePerLevel), so neither is
	// ever scaled twice.
	const float Ratio = EffectMagnitudeRatio + FMath::Max(0, GetAbilityLevel()) * EffectMagnitudeRatioPerLevel;
	const float Flat = EffectValue + FMath::Max(0.f, EventMagnitude) * Ratio;

	// Nothing to grant (a 0-damage hit on a pure-ratio payout, or a row with no
	// flat value at all): skip. Applying a zero-magnitude duration effect would
	// still create an empty buff.
	if (FMath::IsNearlyZero(Flat) && FMath::IsNearlyZero(EffectValuePerLevel))
	{
		return;
	}

	FSnItemStatModifier Modifier;
	Modifier.Attribute = EffectAttribute;
	Modifier.Value = Flat;
	Modifier.ValuePerLevel = EffectValuePerLevel;

	// Reuses the equip path on purpose: it maps the attribute onto its
	// SetByCaller key, zero-fills the generic effect's other keys (no engine
	// warning per unused modifier), scales by level and overrides the duration
	// when one is configured.
	USnItemLibrary::ApplyItemModifiers(TargetASC, EffectClass, { Modifier }, GetAbilityLevel(), EffectDurationSeconds);
}

void USnGA_EventProc::ApplyDamagePayload(FGameplayTag EventTag, float EventMagnitude, const FSnGameplayEffectContext* EventContext, AActor* TargetActor)
{
	// Master gate, see ApplyEffectPayload.
	if (!bEnableDamagePayload || !DamageEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar)
	{
		return;
	}

	// Self-damage is never wanted: it would make the passive its own damage
	// source (and the riposte gate the only thing standing between it and a
	// loop). Configured as the default Self mode with a damage payload set, it
	// is a misconfiguration - skipped here rather than rejected at config time,
	// so blueprint tuning stays cheap.
	if (!TargetActor || TargetActor == Avatar)
	{
		return;
	}

	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);
	if (!TargetASC)
	{
		return;
	}

	// Element: follow the hit that raised the event when asked to. Only the two
	// damage events carry a real element (a heal's context holds the default
	// one), so the fallback keeps a DamageType-configured payload honest on
	// every other event.
	const EDamageType Element = (bUseIncomingElement && IsDamageEvent(EventTag) && EventContext)
		? EventContext->GetDamageType()
		: DamageType;

	// Same split as the effect payload: the flat half grows the equip-row way
	// (Value + Level x ValuePerLevel), the share the damage-family way through
	// the shared formula. Neither is scaled twice.
	const int32 Level = FMath::Max(0, GetAbilityLevel());
	const float Total = DamageValue + Level * DamageValuePerLevel
		+ ScaleDamageByLevel(FMath::Max(0.f, EventMagnitude), DamageRatio, DamageRatioPerLevel, Level);
	if (Total <= 0.f)
	{
		return;
	}

	FSnDamageResult Damage;
	Damage.DamageType = Element;
	Damage.Damage = Total;

	// A damage payload answering Event.DamageTaken IS a riposte, and the flag
	// rides the spec's effect context so the victim's own copy of this passive
	// can recognise it (see the class doc's termination note). Damage raised by
	// any other event is a fresh hit and stays answerable - an enchanted weapon
	// must not make its victim un-retaliable.
	Damage.bReflected = EventTag.MatchesTagExact(SnGameplayTags::Event_DamageTaken);

	// The standard pipeline: ExecCalc mitigation, floating number, kill credit
	// (the avatar is the instigator, so a lethal riposte credits the listener).
	USnCombatLibrary::ApplyDamageToTarget(SourceASC, DamageEffectClass, Avatar, TargetASC, Damage, GetAbilityLevel());
}

void USnGA_EventProc::ReleaseTriggeredAbility()
{
	// The handle only exists when the payload was enabled at activation (see
	// ActivateAbility); the switch is re-read anyway so a blueprint flipping it
	// at runtime cannot release a child the panel says is off.
	if (!bEnableAbilityPayload || !TriggeredAbilityHandle.IsValid())
	{
		return;
	}

	// A refusal - the child is on its own cooldown, or an InstancedPerActor
	// child is still active from the previous payout - skips silently: the
	// count was spent, nothing queues up (the timed-trigger convention).
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
	{
		ASC->TryActivateAbility(TriggeredAbilityHandle);
	}
}

FText USnGA_EventProc::GetDescription_Implementation(int32 Level)
{
	const int32 SafeLevel = FMath::Max(0, Level);
	const FString TargetWord = TargetWordFor(TriggerEventTags, PayloadTargetMode);
	const FString TargetPrefix = TargetWord.IsEmpty() ? FString() : FString::Printf(TEXT("对%s"), *TargetWord);
	TArray<FString> PayloadTexts;

	// Payload A: the effect, described the way an equip row is - catalog
	// display name of the attribute plus the value at Level. Only switched-on
	// payloads are described: the text promises what will actually happen.
	if (bEnableEffectPayload && EffectClass && EffectAttribute.IsValid())
	{
		const float Flat = EffectValue + SafeLevel * EffectValuePerLevel;
		const float Ratio = EffectMagnitudeRatio + SafeLevel * EffectMagnitudeRatioPerLevel;
		FString ValueText = PayloadValueText(Flat, Ratio, EffectMagnitudeRatioPerLevel);
		if (!ValueText.IsEmpty())
		{
			if (EffectDurationSeconds > 0.f)
			{
				ValueText += FString::Printf(TEXT("，持续 %g 秒"), EffectDurationSeconds);
			}

			const FString EffectText = FString::Printf(TEXT("%s +%s"), *AttributeDisplayName(EffectAttribute), *ValueText);
			PayloadTexts.Add(TargetPrefix.IsEmpty() ? EffectText : FString::Printf(TEXT("%s施加 %s"), *TargetPrefix, *EffectText));
		}
	}

	// Payload B: the damage payload, one element through the standard pipeline.
	// The element is named only when it is fixed - a payload following the
	// incoming hit must not advertise one it will not use.
	if (bEnableDamagePayload && DamageEffectClass)
	{
		const float Flat = DamageValue + SafeLevel * DamageValuePerLevel;
		const float Ratio = DamageRatio + SafeLevel * DamageRatioPerLevel;
		const FString ValueText = PayloadValueText(Flat, Ratio, DamageRatioPerLevel);
		if (!ValueText.IsEmpty())
		{
			FString ElementText;
			if (!bUseIncomingElement)
			{
				ElementText = FString::Printf(TEXT("<%s>%s</>"),
					*USnCombatLibrary::GetDamageTypeStyleTag(DamageType).ToString(),
					*USnCombatLibrary::GetDamageTypeDisplayName(DamageType).ToString());
			}

			const FString DamageText = FString::Printf(TEXT("造成 %s 的%s伤害"), *ValueText, *ElementText);
			PayloadTexts.Add(TargetPrefix.IsEmpty() ? DamageText : FString::Printf(TEXT("%s%s"), *TargetPrefix, *DamageText));
		}
	}

	// Payload C: the child's own description text - the numbers live in one
	// place (the child asset) and every proc referencing it stays in sync, the
	// same composition the timed-trigger family uses.
	if (bEnableAbilityPayload && TriggeredAbilityClass)
	{
		if (USnGameplayAbility* ChildCDO = GetMutableDefault<USnGameplayAbility>(TriggeredAbilityClass))
		{
			const FText ChildText = ChildCDO->GetDescription(Level);
			if (!ChildText.IsEmpty())
			{
				PayloadTexts.Add(ChildText.ToString());
			}
		}
	}

	// Nothing configured (or nothing that actually does anything): hand back the
	// static text so the panel still shows something.
	if (PayloadTexts.Num() == 0)
	{
		return Description;
	}

	const FString PayloadText = FString::Join(PayloadTexts, TEXT("，"));
	const FText TriggerWord = FText::FromString(TriggerWordFor(TriggerEventTags, TriggerDamageTypes));
	const int32 Count = FMath::Max(1, EventsPerTrigger);
	if (Count <= 1)
	{
		return FText::Format(INVTEXT("每次{0}：{1}"), TriggerWord, FText::FromString(PayloadText));
	}

	return FText::Format(INVTEXT("每 {0} 次{1}：{2}"), FText::AsNumber(Count), TriggerWord, FText::FromString(PayloadText));
}
