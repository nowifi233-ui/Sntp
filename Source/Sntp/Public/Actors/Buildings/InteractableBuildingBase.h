// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Buildings/BuildingBase.h"
#include "InteractableBuildingBase.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API AInteractableBuildingBase : public ABuildingBase, public IInteractable
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override { return BuildingName;}
	virtual EInteractionType GetType() override { return EInteractionType::Building; };
	
	UFUNCTION(BlueprintImplementableEvent)
	void Blueprint_Interact(AActor* Interactor, FName OptionName);
private:
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> Options;
	
};
