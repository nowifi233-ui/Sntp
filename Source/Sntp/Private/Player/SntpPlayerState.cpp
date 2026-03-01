// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SntpPlayerState.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"

ASntpPlayerState::ASntpPlayerState()
{
	SetNetUpdateFrequency(100.f);
	AbilitySystemComponent = CreateDefaultSubobject<USntpAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 初始化属性集
	AttributeSet = CreateDefaultSubobject<USntpAttributeSet>(TEXT("AttributeSet"));
	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());
}

UAbilitySystemComponent* ASntpPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* ASntpPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}
