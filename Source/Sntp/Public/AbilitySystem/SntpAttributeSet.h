// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "GameplayEffectExtension.h"
#include "SntpAttributeSet.generated.h"


USTRUCT()
struct FEffectProperties
{
	GENERATED_BODY()
	
	FEffectProperties(){}
	
	FGameplayEffectContextHandle EffectContextHandle;
	
	UPROPERTY()
	UAbilitySystemComponent* SourceASC = nullptr;
	
	UPROPERTY()
	AActor* SourceAvatarActor = nullptr;
	
	UPROPERTY()
	AController* SourceController = nullptr;
	
	UPROPERTY()
	ACharacter* SourceCharacter = nullptr;
	
	UPROPERTY()
	UAbilitySystemComponent* TargetASC;
	
	UPROPERTY()
	AActor* TargetAvatarActor;
	
	UPROPERTY()
	AController* TargetController;
	
	UPROPERTY()
	ACharacter* TargetCharacter;
};
/**
 * 
 */

UCLASS()
class SNTP_API USntpAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	USntpAttributeSet();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData &Data) override;
	
	// Attribute Data
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category="Vital Attributes")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, Health)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category="Vital Attributes")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, MaxHealth)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Mana, Category="Vital Attributes")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, Mana)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxMana, Category="Vital Attributes")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, MaxMana)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Resilience, Category="Vital Attributes")
	FGameplayAttributeData Resilience;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, Resilience)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxResilience, Category="Vital Attributes")
	FGameplayAttributeData MaxResilience;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, MaxResilience)
	
	// 
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Attack, Category="Vital Attributes")
	FGameplayAttributeData Attack;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, Attack)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Defense, Category="Vital Attributes")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, Defense)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_ElementalMastery, Category="Vital Attributes")
	FGameplayAttributeData ElementalMastery;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, ElementalMastery)
	
	//
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalChance, Category="Primary Attributes")
	FGameplayAttributeData CriticalChance;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, CriticalChance)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_CriticalDamage, Category="Primary Attributes")
	FGameplayAttributeData CriticalDamage;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, CriticalDamage)
	
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_DamageBonus, Category="Primary Attributes")
	FGameplayAttributeData DamageBonus;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, DamageBonus)
	
	/**
	 * Meta Attributes
	 */
	UPROPERTY(BlueprintReadOnly, Category="Meta Attributes")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS_BASIC(USntpAttributeSet, IncomingDamage)
	
	
	// OnReps
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_Mana(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_MaxMana(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_Resilience(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_MaxResilience(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_CriticalChance(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_CriticalDamage(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_DamageBonus(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_Attack(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_Defense(const FGameplayAttributeData& OldValue) const;
	
	UFUNCTION()
	void OnRep_ElementalMastery(const FGameplayAttributeData& OldValue) const;
	
private:
	void SetEffectProperties(const FGameplayEffectModCallbackData& Data, FEffectProperties& Props);
	void ShowFloatingText(const FEffectProperties& Props,const float Damage, const bool bCritical);
};
