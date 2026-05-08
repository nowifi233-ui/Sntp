// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/FItemDropPair.h"
#include "Actors/Buildings/BuildingBase.h"
#include "Actors/Pickups/PickupActor.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "DestructibleBuilding.generated.h"

/**
 * 
 */


UCLASS()
class SNTP_API ADestructibleBuilding : public ABuildingBase
{
	GENERATED_BODY()
	
public:
	ADestructibleBuilding();
	
	void BuildingTakeDamage(float Damage);
	
	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintReadWrite)
	float MaxResilience = 100.f;
	
	UPROPERTY(BlueprintReadWrite)
	float CurrentResilience = 100.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UWidgetComponent> HealthBarComponent;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeSignature OnResilienceChanged;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeSignature OnMaxResilienceChanged;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FItemDropPair> DropItemDefs;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<APickupActor> PickupActorClass;
	
	UFUNCTION(BlueprintCallable)
	void Drop();
};
