// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PreviewActor.generated.h"

UCLASS()
class SNTP_API APreviewActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	APreviewActor();

	void SetValid(bool bValid);
	
protected:
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* Mesh;
	
	UPROPERTY()
	UMaterialInstanceDynamic* DynamicMaterial;
};
