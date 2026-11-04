// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SnCombatWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "GameplayEffect.h"

void USnCombatWidgetController::BroadcastInitialValues()
{
	if (!AttributeSet)
	{
		return;
	}

	OnHealthChanged.Broadcast(AttributeSet->GetHealth());
	OnMaxHealthChanged.Broadcast(AttributeSet->GetMaxHealth());
	OnPhysicalDamageChanged.Broadcast(AttributeSet->GetPhysicalDamage());
	OnMPChanged.Broadcast(AttributeSet->GetMP());
	OnMaxMPChanged.Broadcast(AttributeSet->GetMaxMP());
	OnMPRegenChanged.Broadcast(AttributeSet->GetMPRegen());
	OnShieldChanged.Broadcast(AttributeSet->GetShield());
	OnXPChanged.Broadcast(AttributeSet->GetXP());
	OnLevelChanged.Broadcast(AttributeSet->GetLevel());
	BroadcastHealthPercent();
	BroadcastXPPercent();
}

void USnCombatWidgetController::BindCallbacks()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetHealthAttribute())
		.AddUObject(this, &USnCombatWidgetController::HealthChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetMaxHealthAttribute())
		.AddUObject(this, &USnCombatWidgetController::MaxHealthChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetPhysicalDamageAttribute())
		.AddUObject(this, &USnCombatWidgetController::PhysicalDamageChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetMPAttribute())
		.AddUObject(this, &USnCombatWidgetController::MPChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetMaxMPAttribute())
		.AddUObject(this, &USnCombatWidgetController::MaxMPChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetMPRegenAttribute())
		.AddUObject(this, &USnCombatWidgetController::MPRegenChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetShieldAttribute())
		.AddUObject(this, &USnCombatWidgetController::ShieldChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetXPAttribute())
		.AddUObject(this, &USnCombatWidgetController::XPChangedCallback);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetLevelAttribute())
		.AddUObject(this, &USnCombatWidgetController::LevelChangedCallback);

	// Attack cooldown: tag count changes when the cooldown effect applies/expires.
	AbilitySystemComponent->RegisterGameplayTagEvent(SnGameplayTags::Cooldown_Attack, EGameplayTagEventType::AnyCountChange)
		.AddUObject(this, &USnCombatWidgetController::AttackCooldownTagChanged);
}

void USnCombatWidgetController::HealthChangedCallback(const FOnAttributeChangeData& Data)
{
	OnHealthChanged.Broadcast(Data.NewValue);
	BroadcastHealthPercent();
}

void USnCombatWidgetController::MaxHealthChangedCallback(const FOnAttributeChangeData& Data)
{
	OnMaxHealthChanged.Broadcast(Data.NewValue);
	BroadcastHealthPercent();
}

void USnCombatWidgetController::PhysicalDamageChangedCallback(const FOnAttributeChangeData& Data)
{
	OnPhysicalDamageChanged.Broadcast(Data.NewValue);
}

void USnCombatWidgetController::MPChangedCallback(const FOnAttributeChangeData& Data)
{
	OnMPChanged.Broadcast(Data.NewValue);
}

void USnCombatWidgetController::MaxMPChangedCallback(const FOnAttributeChangeData& Data)
{
	OnMaxMPChanged.Broadcast(Data.NewValue);
}

void USnCombatWidgetController::MPRegenChangedCallback(const FOnAttributeChangeData& Data)
{
	OnMPRegenChanged.Broadcast(Data.NewValue);
}

void USnCombatWidgetController::ShieldChangedCallback(const FOnAttributeChangeData& Data)
{
	OnShieldChanged.Broadcast(Data.NewValue);
}

void USnCombatWidgetController::XPChangedCallback(const FOnAttributeChangeData& Data)
{
	OnXPChanged.Broadcast(Data.NewValue);
	BroadcastXPPercent();
}

void USnCombatWidgetController::LevelChangedCallback(const FOnAttributeChangeData& Data)
{
	OnLevelChanged.Broadcast(Data.NewValue);
	BroadcastXPPercent();
}

void USnCombatWidgetController::AttackCooldownTagChanged(const FGameplayTag Tag, int32 NewCount)
{
	if (NewCount > 0)
	{
		// Cooldown started: report its total duration so the UI can animate
		// from full to empty. Match on OwningTagQuery: since UE 5.3
		// EffectTagQuery only matches asset tags, while the cooldown tag is a
		// granted tag ("Grant Tags to target actor") - OwningTagQuery covers
		// both. Same query the engine's own ability cooldown check uses.
		const FGameplayEffectQuery Query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(FGameplayTagContainer(Tag));
		const TArray<float> Remaining = AbilitySystemComponent->GetActiveEffectsTimeRemaining(Query);
		if (Remaining.Num() == 0)
		{
			// The tag changed but no active effect carries it: the cooldown GE
			// is not duration-based (Instant, or Duration 0).
			UE_LOG(LogTemp, Warning, TEXT("Attack cooldown tag changed but no active cooldown effect found - check GE_AttackCooldown: Duration Policy must be 'Has Duration'."));
			return;
		}
		OnAttackCooldownChanged.Broadcast(Remaining[0]);
	}
	else
	{
		OnAttackCooldownChanged.Broadcast(0.f);
	}
}

float USnCombatWidgetController::GetHealthPercent() const
{
	if (!AttributeSet)
	{
		return 0.f;
	}

	const float MaxHealth = AttributeSet->GetMaxHealth();
	return MaxHealth > 0.f ? AttributeSet->GetHealth() / MaxHealth : 0.f;
}

float USnCombatWidgetController::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float USnCombatWidgetController::GetMaxHealth() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

float USnCombatWidgetController::GetPhysicalDamage() const
{
	return AttributeSet ? AttributeSet->GetPhysicalDamage() : 0.f;
}

float USnCombatWidgetController::GetMP() const
{
	return AttributeSet ? AttributeSet->GetMP() : 0.f;
}

float USnCombatWidgetController::GetMaxMP() const
{
	return AttributeSet ? AttributeSet->GetMaxMP() : 0.f;
}

float USnCombatWidgetController::GetMPRegen() const
{
	return AttributeSet ? AttributeSet->GetMPRegen() : 0.f;
}

float USnCombatWidgetController::GetShield() const
{
	return AttributeSet ? AttributeSet->GetShield() : 0.f;
}

float USnCombatWidgetController::GetShieldPercent() const
{
	if (!AttributeSet)
	{
		return 0.f;
	}

	// Normalized against MaxHealth, not against a maximum of its own: the pool
	// is uncapped by design, so the health bar's own scale is the only
	// reference the player already has. Not clamped to 1 - a shield bigger
	// than the health bar filling up is exactly the intended readout.
	const float MaxHealth = AttributeSet->GetMaxHealth();
	return MaxHealth > 0.f ? AttributeSet->GetShield() / MaxHealth : 0.f;
}

void USnCombatWidgetController::BroadcastHealthPercent() const
{
	OnHealthPercentChanged.Broadcast(GetHealthPercent());
}

float USnCombatWidgetController::GetXP() const
{
	return AttributeSet ? AttributeSet->GetXP() : 0.f;
}

float USnCombatWidgetController::GetLevel() const
{
	return AttributeSet ? AttributeSet->GetLevel() : 0.f;
}

float USnCombatWidgetController::GetMaxXP() const
{
	// The formula clamps the level to >= 1 internally, so level 0 means 100 XP.
	return USnAttributeSet::GetXPRequiredForLevel(FMath::TruncToInt(GetLevel()));
}

float USnCombatWidgetController::GetXPPercent() const
{
	// Guard like the other getters: on a networked client the controller is
	// created from HUD BeginPlay (OnRep_ReplicatedHasBegunPlay), which can race
	// the ASC's SpawnedAttributes replication - AttributeSet is null until it
	// arrives. Pull returns 0 here; the push delegates refresh the UI as soon
	// as the attributes replicate.
	const float MaxXP = GetMaxXP();
	return (AttributeSet && MaxXP > 0.f) ? AttributeSet->GetXP() / MaxXP : 0.f;
}

void USnCombatWidgetController::BroadcastXPPercent() const
{
	OnXPPercentChanged.Broadcast(GetXPPercent());
}
