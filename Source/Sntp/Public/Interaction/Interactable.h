// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};


USTRUCT(BlueprintType)
struct FInteractionOption
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere)
	FName OptionName;
	
	UPROPERTY(EditAnywhere)
	FText DisplayText;
};

/**
 * 
 */
class SNTP_API IInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual TArray<FInteractionOption> GetInteractionOptions(AActor* Interactor) = 0;
	virtual void Interact(AActor* Interactor, FName OptionName) = 0;
};
