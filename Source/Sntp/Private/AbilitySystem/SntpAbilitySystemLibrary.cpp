// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SntpAbilitySystemLibrary.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "Game/SntpGameModeBase.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/GarbageCollectionSchema.h"

void USntpAbilitySystemLibrary::InitializeDefaultAttributes(const UObject* WorldContextObject,
                                                            ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	ASntpGameModeBase* SntpGameMode = Cast<ASntpGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (SntpGameMode == nullptr)
	{
		return;
	}
	UCharacterClassInfo* ClassInfo = SntpGameMode->CharacterClassInfo;
	const FCharacterClassDefaultInfo CharacterClassInfo = ClassInfo->GetClassDefaultInfo(CharacterClass);
	
	const FGameplayEffectSpecHandle PrimaryEffectSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo.PrimaryAttribute, Level, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryEffectSpecHandle.Data.Get());
	
	const FGameplayEffectSpecHandle VitalEffectSpecHandle = ASC->MakeOutgoingSpec(ClassInfo->VitalAttribute , Level, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*VitalEffectSpecHandle.Data.Get());
}

void USntpAbilitySystemLibrary::GiveStartupAbilites(const UObject* WorldContextObject, UAbilitySystemComponent* ASC)
{
	ASntpGameModeBase* SntpGameMode = Cast<ASntpGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (SntpGameMode == nullptr)
	{
		return;
	}
	UCharacterClassInfo* ClassInfo = SntpGameMode->CharacterClassInfo;
	for (TSubclassOf<UGameplayAbility> AbilityClass : ClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}
}
