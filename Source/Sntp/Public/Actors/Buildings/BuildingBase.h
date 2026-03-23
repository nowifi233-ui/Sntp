// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "BuildingBase.generated.h"

UCLASS()
class SNTP_API ABuildingBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ABuildingBase();
	
	UFUNCTION(BlueprintCallable)
	virtual void Recall(ACharacter* Player);

	UPROPERTY()
	UStaticMesh* Mesh;
protected:
	UPROPERTY(EditAnywhere)
	float Health;
	
	UPROPERTY(EditAnywhere)
	bool bCanRecall;
	
	UPROPERTY(EditAnywhere)
	FName BuildingName;
};
