// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/BuildingBase.h"
#include "Interaction/Interactable.h"
#include "ZiplineNode.generated.h"

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
	
	virtual TArray<FInteractionOption> GetInteractionOptions(AActor* Interactor) override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	float ConnectRadius = 8000.f;
};
