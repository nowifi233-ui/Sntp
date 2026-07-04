// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpEnemyCharacter.h"

#include "SntpGameplayTags.h"
#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAbilitySystemLibrary.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Actors/Pickups/PickupActor.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "MonsterPool/MonsterPoolSubsystem.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "UI/Widgets/SntpUserWidget.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Perception/AIPerceptionComponent.h"

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
	
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();
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
	
	CachedAIController = Cast<AAIController>(GetController());
	
	InitAbilityActorInfo();
	AddCharacterAbilities();
	USntpAbilitySystemLibrary::GiveStartupAbilites(this, AbilitySystemComponent);
	
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
		// Resilience
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetMaxResilienceAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnMaxResilienceChange.Broadcast(Data.NewValue);
		});
		
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(SntpAttributeSet->GetResilienceAttribute()).AddLambda(
		[this](const FOnAttributeChangeData& Data)
		{
			OnResilienceChange.Broadcast(Data.NewValue);
		});
			
		
		FSntpGameplayTags SntpGameplayTags = FSntpGameplayTags::Get();
		AbilitySystemComponent->RegisterGameplayTagEvent(SntpGameplayTags.Effects_HitReact, EGameplayTagEventType::NewOrRemoved).AddUObject(
			this,
			&ASntpEnemyCharacter::HitReactTagChanged);
		
		OnHealthChange.Broadcast(SntpAttributeSet->GetHealth());
		OnMaxHealthChange.Broadcast(SntpAttributeSet->GetMaxHealth());
		OnResilienceChange.Broadcast(SntpAttributeSet->GetResilience());
		OnMaxResilienceChange.Broadcast(SntpAttributeSet->GetMaxResilience());
	}
}

void ASntpEnemyCharacter::Die()
{
	SetLifeSpan(5.f);
	Super::Die();
	
	UGameInstance* GI = GetGameInstance();
	if (GI)
	{
		if (auto Pool = GI->GetSubsystem<UMonsterPoolSubsystem>())
		{
			Pool->ReleaseMonster(this);
		}
	}
}

void ASntpEnemyCharacter::HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount)
{
	bHitReacting = NewCount > 0;
	GetCharacterMovement()->MaxWalkSpeed = bHitReacting? 0.f : BaseWalkSpeed;
	
}

void ASntpEnemyCharacter::ActivateMonster(FVector Location)
{
	bActive = true;
	SetActorLocation(Location);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
	SetActorTickEnabled(true);
	
	AAIController* AI = Cast<AAIController>(GetController());
	if (!AI)
	{
		AI = GetWorld()->SpawnActor<AAIController>(AAIController::StaticClass());
		AI->Possess(this);
	}
	else if (AI->GetPawn() != this)
	{
		AI->Possess(this);
	}
}

void ASntpEnemyCharacter::DeactivateMonster()
{
	bActive = false;
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
	SetActorTickEnabled(false);
	if (CachedAIController)
	{
		
		CachedAIController->ClearFocus(EAIFocusPriority::Gameplay);
		CachedAIController->StopMovement();
	}
}

void ASntpEnemyCharacter::ResetAIState()
{
	if (!CachedAIController) return;
	UBlackboardComponent* BB = CachedAIController->GetBlackboardComponent();
	if (BB)
	{
		// Clear All Values
	}
}

void ASntpEnemyCharacter::InitAbilityActorInfo()
{
	AbilitySystemComponent->InitAbilityActorInfo(this, this);
	Cast<USntpAbilitySystemComponent>(AbilitySystemComponent)->AbilityActorInfoSet();
	
	InitializeDefaultAttributes();
}

void ASntpEnemyCharacter::InitializeDefaultAttributes() const
{
	USntpAbilitySystemLibrary::InitializeDefaultAttributes(this, CharacterClass, Level, AbilitySystemComponent);
}

void ASntpEnemyCharacter::Drop()
{
	if (DropItemDefs.IsEmpty())
	{
		return;
	}
	FTransform SpawnTransform;
	SpawnTransform.SetLocation(GetActorLocation() + FVector(0, 0, 30.0f));
	SpawnTransform.SetRotation(GetActorRotation().Quaternion());
	// Spawn
	for (auto DropItemDef : DropItemDefs)
	{
		APickupActor* NewPickup = GetWorld()->SpawnActorDeferred<APickupActor>(
		PickupActorClass,
		SpawnTransform,
		this,
		nullptr,
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
	);
		if (NewPickup)
		{
			NewPickup->SetItem(DropItemDef.ItemDef, DropItemDef.Amount);
		}
		UGameplayStatics::FinishSpawningActor(NewPickup, SpawnTransform);
	}
	
}
