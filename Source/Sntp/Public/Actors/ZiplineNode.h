// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/BuildingBase.h"
#include "Interaction/Interactable.h"
#include "ZiplineNode.generated.h"

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
	virtual UWidgetComponent* GetInteractionWidget();
	
	UFUNCTION(BlueprintCallable)
	void ShowInteractionWidget();
	
	UFUNCTION(BlueprintCallable)
	void HideInteractionWidget();
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	float ConnectRadius = 8000.f;
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UWidgetComponent> InteractionWidget;
};
