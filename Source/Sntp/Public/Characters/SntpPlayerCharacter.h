// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "InventoryComponent.h"
#include "Characters/SntpCharacterBase.h"
#include "Components/InteractionComponent.h"
#include "Interaction/CombatInterface.h"
#include "SntpPlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerCharacter : public ASntpCharacterBase, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ASntpPlayerCharacter();
	virtual void PossessedBy(AController* byController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	
	// Interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UInteractionComponent> InteractionComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
protected:
	virtual void InitAbilityActorInfo() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TObjectPtr<USphereComponent> InteractionSphere;
	
};
