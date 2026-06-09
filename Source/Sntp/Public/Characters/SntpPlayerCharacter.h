// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InventoryComponent.h"
#include "Characters/SntpCharacterBase.h"
#include "Components/CraftingComponent/CraftingComponent.h"
#include "Components/InteractionComponent.h"
#include "Components/BuildingComponent/BuildableCharacterInterface.h"
#include "Components/FishingComponent/FishingComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "SntpPlayerCharacter.generated.h"

class USntpUserWidget;
class UComboDataAsset;
class UComboComponent;
class UBuildableManagerComponent;
class UCameraComponent;
/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNextLineDelegate);

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
	
	// Virtual Functions of Character
	
	virtual void PossessedBy(AController* byController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual void BeginPlay() override;

	/**
	 * Components:
	 * InteractionComponent / Inventory Component / Crafting Component / Buildable Component / Combo Component
	 */
	
	// Interaction Component
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UInteractionComponent> InteractionComponent;
	
protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UCapsuleComponent> InteractionSphere;
	
	// Inventory Component
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UInventoryComponent> InventoryComponent;
	
	// Crafting Component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UCraftingComponent> CraftingComponent;

	// Buildable Component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UBuildableManagerComponent> BuildManager;
	
	UFUNCTION(BlueprintCallable)
	virtual void DestroyBuildableComponent_Implementation(UStaticMeshComponent* BuildableMesh) override;

	// Fishing Component
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="InteractionComponent")
	TObjectPtr<UFishingComponent> FishingComponent;
	
	/**
	 * Dialogue
	 */

	UPROPERTY(BlueprintAssignable, BlueprintReadWrite)
	FNextLineDelegate NextLineDelegate;
	
	UFUNCTION(BlueprintCallable)
	void NextDialogueLine();
	
	UFUNCTION(BlueprintCallable)
	void StopDialogue();
	
	UFUNCTION(BlueprintCallable)
	void Dialogue(FName Name, FText Text);
	
protected:
	virtual void InitAbilityActorInfo() override;

	/**
	 * Combo Component
	 */
public:
	UFUNCTION(BlueprintCallable)
	UComboComponent* GetComboComponent() const {return ComboComponent;}
protected:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	UComboComponent* ComboComponent;
	
	/* Weapon Dissolve */
	
public:
	UFUNCTION(BlueprintCallable)
	void DissolveWeapon();
	
	UFUNCTION(BlueprintImplementableEvent)
	void StartWeaponDissolveTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);
	
	UFUNCTION(BlueprintCallable)
	void SpawnWeapon();
	
	UFUNCTION(BlueprintImplementableEvent)
	void StartSpawnWeaponTimeline(UMaterialInstanceDynamic* DynamicMaterialInstance);
	
	UPROPERTY(EditAnywhere)
	bool bWeaponSpawned = false;
	
	UFUNCTION(BlueprintCallable)
	void SetIgnoreMoveInput(bool IgnoreMoveInput);
};
