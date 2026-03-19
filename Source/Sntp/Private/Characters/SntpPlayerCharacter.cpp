// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpPlayerCharacter.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Player/SntpPlayerController.h"
#include "Player/SntpPlayerState.h"
#include "UI/SntpHUD.h"

ASntpPlayerCharacter::ASntpPlayerCharacter()
{
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(150.f);
	// ⭐ 只检测Query，不要物理
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>("InteractionComponent");
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	InventoryComponent->InitInventory();
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

void ASntpPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	InteractionComponent->InitSphere(InteractionSphere);
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
