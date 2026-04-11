// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BuildableCharacterInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBuildableCharacterInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SNTP_API IBuildableCharacterInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void DestroyBuildableComponent(UStaticMeshComponent* BuildableMesh);
};
