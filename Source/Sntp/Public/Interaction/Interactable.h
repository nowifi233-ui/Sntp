// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/WidgetComponent.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStateChangeDelegate);


/**
 * FInteractionOptions: Buildings and Pickups
 * 
 */
UENUM(BlueprintType)
enum class EInteractionType : uint8
{
	Building,
	Pickup
};

USTRUCT(BlueprintType)
struct FInteractionOption
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FName OptionName;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FText DisplayText;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UTexture2D> Icon = nullptr;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 Count = 1;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EInteractionType Type;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<AActor> SourceActor;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ItemRarity;
};

/**
 * 
 */
class SNTP_API IInteractable
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
	virtual TArray<FInteractionOption> GetInteractionOptions() = 0;
	virtual void Interact(AActor* Interactor, FName OptionName) = 0;
	virtual FName GetInteractionName() = 0;
	virtual EInteractionType GetType() = 0;
	FStateChangeDelegate StateChangeDelegate;
	
};
