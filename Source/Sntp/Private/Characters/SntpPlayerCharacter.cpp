// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpPlayerCharacter.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Player/SntpPlayerController.h"
#include "Player/SntpPlayerState.h"
#include "UI/SntpHUD.h"

ASntpPlayerCharacter::ASntpPlayerCharacter()
{
	// Set up Cameras
	
	// Follow Camera
	SpringArm = CreateDefaultSubobject<USpringArmComponent>("SpringArm");
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 300.0f;
	SpringArm->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(SpringArm);
	FollowCamera->bUsePawnControlRotation = false;
	
	// Build Camera
	BuildCameraRoot = CreateDefaultSubobject<UCameraComponent>("BuildCameraRoot");
	BuildCameraRoot->SetupAttachment(RootComponent);
	
	BuildCameraRoot->SetRelativeLocation(FVector(0, 0, 0));
	BuildCameraRoot->SetRelativeRotation(FRotator(-60.f, 0, 0));
	
	BuildCamera = CreateDefaultSubobject<UCameraComponent>("BuildCamera");
	BuildCamera->SetupAttachment(BuildCameraRoot);
	BuildCamera->SetRelativeLocation(FVector(0, 0, 1500));
	
	//
	InteractionSphere = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetCapsuleRadius(150.f);
	InteractionSphere->SetCapsuleHalfHeight(100.f);
	
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	// ✅ 打开你需要的重叠通道（最关键）
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap); 
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); 
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);         
	
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>("InteractionComponent");
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	InventoryComponent->InitInventory();
	
	// 
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>("CraftingComponent");
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
	CraftingComponent->Init(InventoryComponent);
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
