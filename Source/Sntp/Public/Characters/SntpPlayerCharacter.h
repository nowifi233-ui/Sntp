// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InventoryComponent.h"
#include "Characters/SntpCharacterBase.h"
#include "Components/CraftingComponent/CraftingComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/BuildingComponent/BuildableCharacterInterface.h"
#include "GameFramework/SpringArmComponent.h"
#include "SntpPlayerCharacter.generated.h"

class UBuildableManagerComponent;
class UCameraComponent;
/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerCharacter : public ASntpCharacterBase, public IAbilitySystemInterface, public IBuildableCharacterInterface
{
	GENERATED_BODY()
public:
	ASntpPlayerCharacter();
	
	// Camera
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	USpringArmComponent* SpringArm;
	
	UPROPERTY(VisibleAnywhere)
	UCameraComponent* FollowCamera;
	
	//
	
	virtual void PossessedBy(AController* byController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;
	
	// Interaction
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UInteractionComponent> InteractionComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	/**
	 * Buildable Component
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UBuildableManagerComponent> BuildManager;
	
	UFUNCTION(BlueprintCallable)
	virtual void DestroyBuildableComponent_Implementation(UStaticMeshComponent* BuildableMesh) override;
protected:
	virtual void InitAbilityActorInfo() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> InteractionSphere;
	
};
