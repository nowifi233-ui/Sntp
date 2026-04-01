// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Buildings/BuildingBase.h"
#include "Components/InventoryComponent.h"
#include "InventoryActorBase.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API AInventoryActorBase : public ABuildingBase, public IInteractable
{
	GENERATED_BODY()
	
public:
	AInventoryActorBase();
	
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override {return BuildingName;}
	virtual EInteractionType GetType() override {return EInteractionType::Building;}
	FStateChangeDelegate StateChangeDelegate;
	
private:
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> Options;
	
	UPROPERTY(EditAnywhere)
	UInventoryComponent* InventoryComponent;
};
