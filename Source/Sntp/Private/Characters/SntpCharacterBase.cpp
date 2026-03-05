// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpCharacterBase.h"

#include "AbilitySystem/SntpAbilitySystemComponent.h"

// Sets default values
ASntpCharacterBase::ASntpCharacterBase()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	
	Weapon = CreateDefaultSubobject<USkeletalMeshComponent>("Weapon");
	Weapon->SetupAttachment(GetMesh(), "WeaponHandSocket");
	Weapon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
}

// Called when the game starts or when spawned
void ASntpCharacterBase::BeginPlay()
{
	Super::BeginPlay();
}

void ASntpCharacterBase::InitAbilityActorInfo()
{
}

void ASntpCharacterBase::InitializePrimaryAttributes()
{
	FGameplayEffectContextHandle EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
	// EffectContextHandle.Get()->AddSourceObject(this);
	FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultPrimaryAttributes, 1.0f, EffectContextHandle);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
}

void ASntpCharacterBase::InitializeVitalAttributes()
{
	FGameplayEffectContextHandle EffectContextHandle = AbilitySystemComponent->MakeEffectContext();
	// EffectContextHandle.Get()->AddSourceObject(this);
	FGameplayEffectSpecHandle EffectSpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultVitalAttributes, 1.0f, EffectContextHandle);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
}

void ASntpCharacterBase::AddCharacterAbilities()
{
	USntpAbilitySystemComponent* SntpASC = CastChecked<USntpAbilitySystemComponent>(AbilitySystemComponent);
	if (!HasAuthority())
	{
		return;
	}
	SntpASC->AddCharacterAbilities(StartupAbilities);
}

// Called to bind functionality to input
void ASntpCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

