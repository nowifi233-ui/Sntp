// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpPlayerCharacter.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Player/SntpPlayerController.h"
#include "Player/SntpPlayerState.h"
#include "UI/SntpHUD.h"

ASntpPlayerCharacter::ASntpPlayerCharacter()
{
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>("InteractionComponent");
}

void ASntpPlayerCharacter::PossessedBy(AController* byController)
{
	Super::PossessedBy(byController);
	// Server
	InitAbilityActorInfo();
	AddCharacterAbilities();
}

void ASntpPlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	// Client
	InitAbilityActorInfo();
}

UAbilitySystemComponent* ASntpPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void ASntpPlayerCharacter::InitAbilityActorInfo()
{
	ASntpPlayerState* SntpPlayerState = GetPlayerState<ASntpPlayerState>();
	check(SntpPlayerState);
	SntpPlayerState->GetAbilitySystemComponent()->InitAbilityActorInfo(SntpPlayerState, this);
	AbilitySystemComponent = SntpPlayerState->GetAbilitySystemComponent();
	Cast<USntpAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
	AttributeSet = SntpPlayerState->GetAttributeSet();
	
	if (ASntpPlayerController* SntpPlayerController = Cast<ASntpPlayerController>(GetController()))
	{
		if (ASntpHUD* SntpHUD = Cast<ASntpHUD>(SntpPlayerController->GetHUD()))
		{
			SntpHUD->InitOverlay(SntpPlayerController, SntpPlayerState, AbilitySystemComponent, AttributeSet);
		}
	}
	
	InitializePrimaryAttributes();
	InitializeVitalAttributes();
}
