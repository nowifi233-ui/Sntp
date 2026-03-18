// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "InteractablePoint.generated.h"

UCLASS()
class SNTP_API AInteractablePoint : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AInteractablePoint();

	
protected:
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override { return BuildingName; }
	virtual EInteractionType GetType() override {return EInteractionType::Building;}
	
	UPROPERTY(EditAnywhere)
	FName BuildingName;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FInteractionOption> Options;
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnInteract1();
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnInteract2();
};
