// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SntpAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "SntpGameplayTags.h"
#include "AbilitySystem/SntpAbilitySystemLibrary.h"
#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "Net/UnrealNetwork.h"
#include "Player/SntpPlayerController.h"


USntpAttributeSet::USntpAttributeSet()
{
	
}

void USntpAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, CriticalChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, Resilience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, MaxResilience, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, CriticalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, DamageBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, Attack, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USntpAttributeSet, ElementalMastery, COND_None, REPNOTIFY_Always);
	
}

void USntpAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	FEffectProperties Props;
	SetEffectProperties(Data, Props);
	
	// clamp health
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		const float NewHealth = FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth());
		SetHealth(NewHealth);
		UE_LOG(LogTemp, Warning, TEXT("Change Health on %s, Health = %f."), *Props.TargetAvatarActor->GetName(), GetHealth());
		if (NewHealth <= 0.f)
		{
			// Character 
			
		}
		
	}
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float LocalIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.f);

		const float NewHealth = GetHealth() - LocalIncomingDamage;
		SetHealth(FMath::Clamp(NewHealth, 0.f, GetMaxHealth()));
		const bool bFatal = NewHealth <= 0.f;
		if (!bFatal)
		{
			FGameplayTagContainer TagContainer;
			FSntpGameplayTags SntpGameplayTags = FSntpGameplayTags::Get();
			TagContainer.AddTag(SntpGameplayTags.Effects_HitReact);
			Props.TargetASC->TryActivateAbilitiesByTag(TagContainer);
		}
		else
		{
			// Die
			ICombatInterface* CombatInterface = Cast<ICombatInterface>(Props.TargetAvatarActor);
			if (CombatInterface)
			{
				CombatInterface->Die();
				CombatInterface->Drop();
			}
		}
			
		const bool bCritical = USntpAbilitySystemLibrary::IsCriticalHit(Props.EffectContextHandle);
		ShowFloatingText(Props, LocalIncomingDamage, bCritical);
	}
}

void USntpAttributeSet::SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props)
{
	// Get information of Gameplay effect.
	Props.EffectContextHandle = Data.EffectSpec.GetContext();
	Props.SourceASC = Props.EffectContextHandle.GetOriginalInstigatorAbilitySystemComponent();
	if (IsValid(Props.SourceASC) && Props.SourceASC->AbilityActorInfo.IsValid() && Props.SourceASC->AbilityActorInfo->AvatarActor.IsValid())
	{
		// Get Source avatar actor
		Props.SourceAvatarActor = Props.SourceASC->AbilityActorInfo->AvatarActor.Get();
		// Get source controller
		Props.SourceController = Props.SourceASC->AbilityActorInfo->PlayerController.Get();
		if (Props.SourceController == nullptr && Props.SourceAvatarActor != nullptr)
		{
			if (APawn* Pawn = Cast<APawn>(Props.SourceAvatarActor))
			{
				Props.SourceController = Pawn->GetController();
			}
		}
		// Get source character
		if (Props.SourceController)
		{
			Props.SourceCharacter = Cast<ACharacter>(Props.SourceController->GetPawn());
		}
	}
	
	// Get Target Actor
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		Props.TargetAvatarActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		Props.TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		if (Props.TargetController == nullptr && Props.TargetAvatarActor != nullptr)
		{
			if (APawn* Pawn = Cast<APawn>(Props.TargetAvatarActor))
			{
				Props.TargetController = Pawn->GetController();
			}
		}
		Props.TargetCharacter = Cast<ACharacter>(Props.TargetController->GetPawn());
		Props.TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Props.TargetAvatarActor);
	}
}

void USntpAttributeSet::ShowFloatingText(const FEffectProperties& Props, const float Damage, const bool bCritical)
{
	if (Props.SourceCharacter != Props.TargetCharacter)
	{
		if (ASntpPlayerController* PC = Cast<ASntpPlayerController>(Props.SourceCharacter->Controller))
		{
			PC->ShowDamageNumber(Damage, Props.TargetCharacter, bCritical);
		}
	}
}

void USntpAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, Health, OldValue);
}

void USntpAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, MaxHealth, OldValue);
}

void USntpAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, Mana, OldValue);
}

void USntpAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, MaxMana, OldValue);
}

void USntpAttributeSet::OnRep_Resilience(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, Resilience, OldValue);
}

void USntpAttributeSet::OnRep_MaxResilience(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, MaxResilience, OldValue);
}

void USntpAttributeSet::OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, CriticalChance, OldValue);
}

void USntpAttributeSet::OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, CriticalDamage, OldValue);
}

void USntpAttributeSet::OnRep_DamageBonus(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, DamageBonus, OldValue);
}

void USntpAttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, Attack, OldValue);
}

void USntpAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, Defense, OldValue);
}

void USntpAttributeSet::OnRep_ElementalMastery(const FGameplayAttributeData& OldValue) const
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USntpAttributeSet, ElementalMastery, OldValue);
}


