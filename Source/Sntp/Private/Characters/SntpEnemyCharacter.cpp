// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpEnemyCharacter.h"

#include "NavigationSystemTypes.h"
#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Components/Widget.h"
#include "Components/WidgetComponent.h"
#include "UI/Widgets/SntpUserWidget.h"

ASntpEnemyCharacter::ASntpEnemyCharacter()
{
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	
	AbilitySystemComponent = CreateDefaultSubobject<USntpAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	
	AttributeSet = CreateDefaultSubobject<USntpAttributeSet>("AttributeSet");
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet.Get());
	
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>("HealthBarComponent");
	HealthBarComponent->SetupAttachment(RootComponent);
}

void ASntpEnemyCharacter::HighlightActor()
{
	GetMesh()->SetRenderCustomDepth(true);
	GetMesh()->SetCustomDepthStencilValue(250);
	Weapon->SetRenderCustomDepth(true);
	Weapon->SetCustomDepthStencilValue(250);
}

void ASntpEnemyCharacter::UnHighlightActor()
{
	GetMesh()->SetRenderCustomDepth(false);
	Weapon->SetRenderCustomDepth(false);
}

void ASntpEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilityActorInfo();
	AddCharacterAbilities();
	
	USntpUserWidget* SntpUserWidget = Cast<USntpUserWidget>(HealthBarComponent->GetUserWidgetObject());
	if (SntpUserWidget)
	{
		SntpUserWidget->SetWidgetController(this);
	}
	
	USntpAttributeSet* SntpAttributeSet = Cast<USntpAttributeSet>(AttributeSet);
	if (SntpAttributeSet)
	{
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetMaxHealthAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
			{
				OnMaxHealthChange.Broadcast(Data.NewValue);
			});
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetHealthAttribute()).AddLambda(
			[this](const FOnAttributeChangeData& Data)
			{
				OnHealthChange.Broadcast(Data.NewValue);
			});
		OnHealthChange.Broadcast(SntpAttributeSet->GetHealth());
		OnMaxHealthChange.Broadcast(SntpAttributeSet->GetMaxHealth());
	}
	
	
}

void ASntpEnemyCharacter::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<USntpAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
	InitializePrimaryAttributes();
	InitializeVitalAttributes();
}
