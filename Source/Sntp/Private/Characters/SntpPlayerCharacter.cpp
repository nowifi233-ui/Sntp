// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpPlayerCharacter.h"

#include "AbilitySystem/SntpAttributeSet.h"
#include "Player/SntpPlayerState.h"

void ASntpPlayerCharacter::PossessedBy(AController* byController)
{
	Super::PossessedBy(byController);
	// Server
	InitAbilityActorInfo();
}

void ASntpPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// Client
	InitAbilityActorInfo();
}

void ASntpPlayerCharacter::InitAbilityActorInfo()
{
	ASntpPlayerState* SntpPlayerState = GetPlayerState<ASntpPlayerState>();
	check(SntpPlayerState);
	SntpPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(SntpPlayerState, this);
	AbilitySystemComponent = SntpPlayerState->GetAbilitySystemComponent();
	AttributeSet = SntpPlayerState->GetAttributeSet();
}
