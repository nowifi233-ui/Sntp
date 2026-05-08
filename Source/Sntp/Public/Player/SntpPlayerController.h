// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "AbilitySystem/SntpAbilitySystemComponent.h"
#include "Actors/Buildings/BuildingBase.h"
#include "SntpPlayerController.generated.h"

class UComboComponent;
class UDamageTextComponent;
class USntpInputConfig;
class IEnemyInterface;

/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ASntpPlayerController();
	virtual void PlayerTick(const float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	
	/**
	 * Enhanced Input Component
	 */
protected:
	virtual void SetupInputComponent() override;
	
private:
	UPROPERTY(EditAnywhere, Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;

	/**
	 * Basic Control:
	 * Include Basic Move and Look
	 */
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;

	void HandleMove(const FInputActionValue& InputValue);
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* LookAction;
	
	void HandleLook(const FInputActionValue& InputValue);
	
	FRotator TargetControlRotation;

	
	/**
	 * Interaction System: 
	 * Scroll / Interaction / Toggle Inventory
	 */
private:
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ScrollAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* InteractAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* ToggleInventoryAction;
	
	void HandleScroll(const FInputActionValue& InputValue);
	void HandleInteract(const FInputActionValue& InputValue);
	void ToggleInventory(const FInputActionValue& InputValue);
	
	/**
	 *  Setting Menu: Handle setting menu;
	 */
private:
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SettingMenuAction;
	
	void HandleSettingMenu(const FInputActionValue& InputValue);

	/**
	 * Ability System Bind
	 */
private:
	UPROPERTY()
	TObjectPtr<USntpAbilitySystemComponent> SntpAbilitySystemComponent;
	
	USntpAbilitySystemComponent* GetASC();
	
	UPROPERTY(EditAnywhere, Category="Input")
	TObjectPtr<USntpInputConfig> InputConfig;
	
	void AbilityInputTagPressed(FGameplayTag InputTag);
	void AbilityInputTagReleased(FGameplayTag InputTag);
	void AbilityInputTagHeld(FGameplayTag InputTag);
	
	/**
	 * Highlight Enemy by Cursor
	 */
private:
	void CursorTrace();
	IEnemyInterface* CurrentActor;
	IEnemyInterface* LastActor;
	
	/** 
	 * Show Damage Text
	 */
public:
	UFUNCTION(Client, Reliable)
	void ShowDamageNumber(float DamageAmount, ACharacter* TargetCharacter, bool bCritical);
	
private:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UDamageTextComponent> DamageTextComponentClass;
	
	/**
	 * Build Component
	 * Switch Build Mode / Place Buildable
	 */
private:
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* SwitchBuildModeAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* PlaceAction;

	UFUNCTION()
	void ToggleBuildMode();
	
	UFUNCTION()
	void HandlePlace();
	
	/**
	 * Combat System
	 */
public:
	UPROPERTY(BlueprintReadWrite)
	bool CanPreInput = false;
	
	UPROPERTY(BlueprintReadWrite)
	FGameplayTag PreInputTag;
	
	UPROPERTY(BlueprintReadWrite)
	bool bIgnoreMoveInput = false;
	
	/**
	 * Press Alt to Show mouse curser
	 */
public:
	bool bUIInteractionMode = false;
	void EnterUIMode();
	void ExitUIMode();
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AltPressedAction;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* AltReleasedAction;
	
	void ToggleUIMode(bool bEnable);
	bool bIsInUIMode = false;
};
