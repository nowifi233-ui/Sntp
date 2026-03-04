// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SntpCharacterBase.h"

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

// Called to bind functionality to input
void ASntpCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

