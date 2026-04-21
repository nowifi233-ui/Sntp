// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ItemDefinition.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "PickupActor.generated.h"

UCLASS()
class SNTP_API APickupActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APickupActor();
	APickupActor(UItemDefinition* InItemDef, int32 InCount);
	
	UFUNCTION(BlueprintCallable)
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override {return ItemName;}
	virtual EInteractionType GetType() override {return EInteractionType::Pickup;}
	virtual void SetItem(UItemDefinition* InItemDef, int32 InCount);
	
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FName ItemName;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UItemDefinition> ItemDef;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 Count = 1;
};
