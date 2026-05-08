// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpPlayerCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "SntpGameplayTags.h"
#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "AbilitySystem/SntpAttributeSet.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BuildingComponent/BuildableManagerComponent.h"
#include "Components/ComboComponent/ComboComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	
	//
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 10.f; 
	//
	InteractionSphere = CreateDefaultSubobject<UCapsuleComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetCapsuleRadius(150.f);
	InteractionSphere->SetCapsuleHalfHeight(100.f);
	
	InteractionSphere->SetGenerateOverlapEvents(true);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	
	InteractionSphere->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);

	//
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Overlap); 
	InteractionSphere->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap); 
	InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);         
	
	InteractionComponent = CreateDefaultSubobject<UInteractionComponent>("InteractionComponent");
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>("InventoryComponent");
	InventoryComponent->InitInventory();
	
	// 
	CraftingComponent = CreateDefaultSubobject<UCraftingComponent>("CraftingComponent");
	
	//
	BuildManager = CreateDefaultSubobject<UBuildableManagerComponent>("BuildManager");
	BuildManager->InitialBuildableManagerComponent(FollowCamera);

	// Combo
	ComboComponent = CreateDefaultSubobject<UComboComponent>("ComboComponent");
}

void ASntpPlayerCharacter::PossessedBy(AController* byController)
{
	Super::PossessedBy(byController);
	// Server
	InitAbilityActorInfo();
	AddCharacterAbilities();
	ComboComponent->Init(AbilitySystemComponent);
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


void ASntpPlayerCharacter::DestroyBuildableComponent_Implementation(UStaticMeshComponent* BuildableMesh)
{
	BuildableMesh->DestroyComponent();
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

void ASntpPlayerCharacter::DissolveWeapon()
{
	if (bWeaponSpawned == false)
	{
		return;
	}
	if (IsValid(WeaponDissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMatInst = UMaterialInstanceDynamic::Create(WeaponDissolveMaterialInstance, this);
		Weapon->SetMaterial(0, DynamicMatInst);
		StartWeaponDissolveTimeline(DynamicMatInst);
	}
	bWeaponSpawned = false;
}

void ASntpPlayerCharacter::SpawnWeapon()
{
	if (bWeaponSpawned == true)
	{
		return;
	}
	if (IsValid(WeaponDissolveMaterialInstance))
	{
		UMaterialInstanceDynamic* DynamicMatInst = UMaterialInstanceDynamic::Create(WeaponDissolveMaterialInstance, this);
		Weapon->SetMaterial(0, DynamicMatInst);
		StartSpawnWeaponTimeline(DynamicMatInst);
	}
	bWeaponSpawned = true;
}

void ASntpPlayerCharacter::SetIgnoreMoveInput(bool IgnoreMoveInput)
{
	ASntpPlayerController* PC = GetController<ASntpPlayerController>();
	if(PC)
	{
		PC->ResetIgnoreMoveInput();
		PC->SetIgnoreMoveInput(IgnoreMoveInput);
		PC->bIgnoreMoveInput = IgnoreMoveInput;
	}
	if (IgnoreMoveInput)
	{
		PC->StopMovement();
	}
}
