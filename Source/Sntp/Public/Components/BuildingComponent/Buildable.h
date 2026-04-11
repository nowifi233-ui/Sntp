// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "Buildable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UBuildable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 
 */
class SNTP_API IBuildable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual FGameplayTag GetBuildableTag() = 0;
	virtual UPrimitiveComponent* GetSocket() = 0;
};
