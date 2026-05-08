// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/BuildingComponent/Buildable.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "BuildingBase.generated.h"

UCLASS()
class SNTP_API ABuildingBase : public AActor, public IBuildable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuildingBase();
	
	UFUNCTION(BlueprintCallable)
	virtual void Recall(ACharacter* Player);

	UPROPERTY()
	UStaticMesh* BuildingMesh;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMeshComponent* BuildingMeshComponent;
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag BuildingTag;
	
	virtual void OnConstruction(const FTransform& Transform) override;
	
	/**
	 * IBuildable
	 */
	virtual FGameplayTag GetBuildableTag() override;
	virtual UPrimitiveComponent* GetSocket() override;
	
protected:
	UPROPERTY(EditAnywhere)
	float Health = 100.f;
	
	UPROPERTY(EditAnywhere)
	bool bCanRecall;
	
	UPROPERTY(EditAnywhere)
	FName BuildingName;
};
