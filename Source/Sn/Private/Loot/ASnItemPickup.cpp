// Fill out your copyright notice in the Description page of Project Settings.

#include "Loot/ASnItemPickup.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Interaction/UInteractableComponent.h"
#include "Loot/USnDropMotionComponent.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ASnItemPickup::ASnItemPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	// Replicated so clients see the dropped item and its data (row name +
	// count arrive via OnRep; interaction state replicates via the
	// interactable component). All pickup logic stays server-side.
	SetReplicates(true);

	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->InitSphereRadius(40.f);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	SetRootComponent(CollisionSphere);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(CollisionSphere);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	// Fallback sphere so the pickup is visible without a configured mesh;
	// BP subclasses override this with the real item visual.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (DefaultMesh.Succeeded())
	{
		Mesh->SetStaticMesh(DefaultMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.25f));
	}

	InteractableComponent = CreateDefaultSubobject<UInteractableComponent>(TEXT("InteractableComponent"));
	// bIsPickup and the pickup row name are two halves of the same system
	// contract: a pickup from ANY source (drop, enemy loot, hand-placed) must
	// be F-interactable out of the box. Subclasses can still override the row
	// for special pickups.
	InteractableComponent->bIsPickup = true;
	InteractableComponent->InteractRowName = FName(TEXT("ItemPickup"));

	DropMotion = CreateDefaultSubobject<USnDropMotionComponent>(TEXT("DropMotion"));
}

void ASnItemPickup::Initialize(FName InItemRowName, int32 InCount)
{
	ItemRowName = InItemRowName;
	Count = FMath::Max(1, InCount);
	OnPickupInitialized();
}

void ASnItemPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASnItemPickup, ItemRowName);
	DOREPLIFETIME(ASnItemPickup, Count);
}

void ASnItemPickup::OnRep_PickupData()
{
	OnPickupInitialized();
}

void ASnItemPickup::ActivateForPool()
{
	// Revive: visible, interactable again, drop motion restarted (LaunchDrop
	// is re-entrant - it re-rolls the velocity and the settle timer).
	SetActorHiddenInGame(false);
	if (InteractableComponent)
	{
		InteractableComponent->bEnabled = true;
	}
	if (DropMotion)
	{
		DropMotion->SetComponentTickEnabled(true);
		DropMotion->LaunchDrop();
	}
}

void ASnItemPickup::DeactivateForPool()
{
	SetActorHiddenInGame(true);
	// A hidden pickup must not show up in the interaction scan - otherwise
	// the wheel/F-key could select an invisible pickup.
	if (InteractableComponent)
	{
		InteractableComponent->bEnabled = false;
	}
	if (DropMotion)
	{
		DropMotion->SetComponentTickEnabled(false);
	}
}

void ASnItemPickup::BeginPlay()
{
	Super::BeginPlay();
	DropMotion->LaunchDrop();
}
