// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/DestructibleBuilding.h"

#include "Kismet/GameplayStatics.h"
#include "UI/Widgets/SntpUserWidget.h"

ADestructibleBuilding::ADestructibleBuilding()
{
	HealthBarComponent = CreateDefaultSubobject<UWidgetComponent>("HealthBarComponent");
	HealthBarComponent->SetupAttachment(BuildingMeshComponent);
}

void ADestructibleBuilding::BuildingTakeDamage(float Damage)
{
	CurrentResilience -= Damage;
	OnResilienceChanged.Broadcast(CurrentResilience);
	if (CurrentResilience <= 0.f)
	{
		Drop();
		Destroy();
	}
}

void ADestructibleBuilding::BeginPlay()
{
	Super::BeginPlay();
	
	USntpUserWidget* SntpUserWidget = Cast<USntpUserWidget>(HealthBarComponent->GetUserWidgetObject());
	if (SntpUserWidget)
	{
		SntpUserWidget->SetWidgetController(this);
	}
	OnMaxResilienceChanged.Broadcast(MaxResilience);
	OnResilienceChanged.Broadcast(CurrentResilience);
}

void ADestructibleBuilding::Drop()
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
