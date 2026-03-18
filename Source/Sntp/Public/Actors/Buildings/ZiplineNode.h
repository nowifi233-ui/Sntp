// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Buildings/BuildingBase.h"
#include "Interaction/Interactable.h"
#include "ZiplineNode.generated.h"

class UGameplayEffect;
class UWidgetComponent;
/**
 * 
 */
UCLASS()
class SNTP_API AZiplineNode : public ABuildingBase, public IInteractable
{
	GENERATED_BODY()
public:
	AZiplineNode();
	
	UPROPERTY()
	TArray<AZiplineNode*> ConnectedNodes;
	
	void FindConnectedNodes();
	
	UFUNCTION(BlueprintCallable)
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override {return BuildingName;}
	virtual EInteractionType GetType() override {return EInteractionType::Building;}
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FInteractionOption> Options;

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	float ConnectRadius = 8000.f;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGameplayEffect> DamageEffectClass;
};
