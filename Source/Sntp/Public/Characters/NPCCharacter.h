// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/DialogueComponent/DialogueComponent.h"
#include "GameFramework/Character.h"
#include "Interaction/Interactable.h"
#include "NPCCharacter.generated.h"

UCLASS()
class SNTP_API ANPCCharacter : public ACharacter, public IInteractable
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ANPCCharacter();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="NPC")
	FName NPCName;
	
	/**
	 * Interaction API
	 */
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact (AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override { return NPCName; }
	virtual EInteractionType GetType() override { return EInteractionType::Building; }
	FStateChangeDelegate StateChangeDelegate;
	
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> Options;
	
	UFUNCTION(BlueprintImplementableEvent)
	void Blueprint_Interact(AActor* Interactor, FName OptionName);
	
	/**
	 * Dialogue Component
	 */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UDialogueComponent> DialogueComponent;
	
	
};
