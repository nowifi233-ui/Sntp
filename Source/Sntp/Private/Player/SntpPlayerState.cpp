// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SntpPlayerState.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Net/UnrealNetwork.h"

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

void ASntpPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ASntpPlayerState, PlayerUid);
	DOREPLIFETIME(ASntpPlayerState, PlayerDisplayName);
}

UAbilitySystemComponent* ASntpPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* ASntpPlayerState::GetAttributeSet() const
{
	return AttributeSet;
}

int64 ASntpPlayerState::GetPlayerUid() const
{
	if (PlayerUid != INDEX_NONE)
	{
		return PlayerUid;
	}
	return  GetPlayerId();
}

const FString& ASntpPlayerState::GetPlayerDisplayName() const
{
	return PlayerDisplayName;
}
