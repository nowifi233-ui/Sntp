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
	ComboComponent->Init(AbilitySystemComponent, ComboDataAsset);
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

void ASntpPlayerCharacter::HitScan()
{
	if (!Weapon) return;
	
	FVector CurrentWeaponStart = Weapon->GetSocketLocation(FName("Start"));
	FVector CurrentWeaponMid = Weapon->GetSocketLocation(FName("Mid"));
	FVector CurrentWeaponEnd = Weapon->GetSocketLocation(FName("End"));
	
	if (!bHasLastFrame)
	{
		LastWeaponStart = CurrentWeaponStart;
		LastWeaponMid = CurrentWeaponMid;
		LastWeaponEnd = CurrentWeaponEnd;
		bHasLastFrame = true;
		return;
	}
	
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.bTraceComplex = false;
	
	float Radius = 50.f;
	
	TArray<FHitResult> Hits;
	
	GetWorld()->SweepMultiByChannel(
		Hits,
		LastWeaponStart,
		CurrentWeaponStart,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params);
	
	GetWorld()->SweepMultiByChannel(
		Hits,
		LastWeaponMid,
		CurrentWeaponMid,
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params);
	
	GetWorld()->SweepMultiByChannel(
		Hits,
		LastWeaponEnd,
		CurrentWeaponEnd, // ⭐ End轨迹
		FQuat::Identity,
		ECC_Pawn,
		FCollisionShape::MakeSphere(Radius),
		Params
	);
	
	
	// Debug
	DrawDebugLine(GetWorld(), LastWeaponStart, CurrentWeaponStart, FColor::Red, false, 1.f, 0, 2);
	DrawDebugLine(GetWorld(), LastWeaponMid, CurrentWeaponMid, FColor::Red, false, 1.f, 0, 2);
	DrawDebugLine(GetWorld(), LastWeaponEnd, CurrentWeaponEnd, FColor::Red, false, 1.f, 0, 2);


	for (const FHitResult& Hit : Hits)
	{
		AActor* HitActor = Hit.GetActor();
		if (!HitActor) continue;
		
		// Deal with HitActor
		// ...
		// ...
		if (HitActors.Contains(HitActor))
		{
			continue;
		}
		HitActors.AddUnique(HitActor);
		
		// Give the projectile a Gameplay Effect to Cause Damage;
		const FGameplayEffectSpecHandle EffectSpecHandle = WeaponTraceEffectHandle;
		
		/*const FSntpGameplayTags GameplayTags = FSntpGameplayTags::Get();
		const float ScaledDamage = 10.f;
		UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, GameplayTags.Damage, ScaledDamage);*/
		
		if (HasAuthority())
		{
			if (UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor))
			{
				if (!EffectSpecHandle.Data)
				{
					return;
				}
				TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
			}
		}
	}
	LastWeaponStart = CurrentWeaponStart;
	LastWeaponMid = CurrentWeaponMid;
	LastWeaponEnd = CurrentWeaponEnd;
}

void ASntpPlayerCharacter::HitScanStart(FGameplayEffectSpecHandle DamageEffectSpecHandle)
{
	WeaponTraceEffectHandle = DamageEffectSpecHandle;
	if (!WeaponTraceEffectHandle.Data)
	{
		return;
	}
	HitActors.Reset();
	
	LastWeaponStart = Weapon->GetSocketLocation(FName("Start"));
	LastWeaponMid = Weapon->GetSocketLocation(FName("Mid"));
	LastWeaponEnd = Weapon->GetSocketLocation(FName("End"));
	bHasLastFrame = false;
	
	GetWorldTimerManager().SetTimer(
		WeaponHitTimer,
		this,
		&ASntpPlayerCharacter::HitScan,
		1.f/60.f,
		true
		);
}

void ASntpPlayerCharacter::HitScanEnd()
{
	GetWorldTimerManager().ClearTimer(WeaponHitTimer);
	bHasLastFrame = false;
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
