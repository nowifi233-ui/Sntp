// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/DialogueComponent/DialogueComponent.h"
#include "UI/Widgets/SntpUserWidget.h"
#include "GameFramework/HUD.h"
#include "WidgetController/CraftingWidgetController.h"
#include "WidgetController/EquipmentWidgetController.h"
#include "WidgetController/InventoryWidgetController.h"
#include "SntpHUD.generated.h"

class UDialogueWidgetController;
class UAttributeSet;
class UAbilitySystemComponent;
struct FWidgetControllerParams;
class UOverlayWidgetController;
/**
 * 
 */
UCLASS()
class SNTP_API ASntpHUD : public AHUD
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	TObjectPtr<USntpUserWidget> OverlayWidget;
	
	UOverlayWidgetController* GetOverlayWidgetController(const FWidgetControllerParams& Params);
	UOverlayWidgetController* GetOverlayWidgetControllerWithoutParam();
	
	UFUNCTION(BlueprintCallable)
	void InitOverlay(APlayerController* PlayerController, APlayerState* PlayerState, UAbilitySystemComponent* AbilitySystemComponent, UAttributeSet* AttributeSet);

	UFUNCTION(BlueprintCallable)
	void ToggleBag(APlayerController* PlayerController);
	
	UFUNCTION(BlueprintCallable)
	void ToggleInventoryUI(APlayerController* PlayerController, UInventoryComponent* TargetInventoryComponent);
	
	UFUNCTION(BlueprintCallable)
	void ToggleSettingMenu(APlayerController* PlayerController);
	
	UFUNCTION(BlueprintCallable)
	void ToggleCraftingWidget(APlayerController* PlayerController);
	
	UFUNCTION(BlueprintCallable)
	void ToggleFishingWidget(APlayerController* PlayerController);
	
	UFUNCTION(BlueprintCallable)
	void ToggleDialogueWidget(APlayerController* PlayerController, UDialogueComponent* InDialogueComponent);
	
	UFUNCTION(BlueprintCallable)
	void ToggleEquipmentWidget(APlayerController* PlayerController);
private:
	/*
	 * Overlay Widget Controller
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> OverlayWidgetClass;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UOverlayWidgetController> OverlayWidgetControllerClass; 
	
	UPROPERTY()
	TObjectPtr<UOverlayWidgetController> OverlayWidgetController;
	
public:
	TSharedPtr<SWidget> OverlaySlateWidget;
	
private:

	/*
	 * Inventory Widget Controller
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> BagWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USntpUserWidget> BagWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UInventoryWidgetController> InventoryWidgetControllerClass; 
	
	UPROPERTY()
	TObjectPtr<UInventoryWidgetController> InventoryWidgetController;

	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> InventoryWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USntpUserWidget> InventoryWidget;
	
	UFUNCTION(BlueprintCallable)
	UInventoryWidgetController* GetInventoryWidgetController(APlayerController* InPlayerController);
	
	/**
	 * Crafting Widget
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> CraftingWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USntpUserWidget> CraftingWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UCraftingWidgetController> CraftingWidgetControllerClass; 
	
	UPROPERTY()
	TObjectPtr<UCraftingWidgetController> CraftingWidgetController;
	
	/**
	 * Setting Menu
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> SettingWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> SettingWidget;
	
	/**
	* Fishing Widget 
	*/ 
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> FishWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USntpUserWidget> FishWidget;
	
	/**
	 * Dialogue Component
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> DialogueWidgetClass;
	
	UPROPERTY()
	TObjectPtr<USntpUserWidget> DialogueWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UDialogueWidgetController> DialogueWidgetControllerClass; 
	
	UPROPERTY()
	TObjectPtr<UDialogueWidgetController> DialogueWidgetController;
	
	/*
	 * Equipment Component
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> EquipmentWidgetClass;

	UPROPERTY()
	TObjectPtr<USntpUserWidget> EquipmentWidget;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<UEquipmentWidgetController> EquipmentWidgetControllerClass; 
	
	UPROPERTY()
	TObjectPtr<UEquipmentWidgetController> EquipmentWidgetController;
	

private:
	bool bBagOpen = false;
	bool bSettingOpen = false;
	bool bInventoryOpen = false;
	bool bCraftingOpen = false;
	bool bIsUIOpen = false;
	bool bFishOpen = false;
	bool bDialogueOpen = false;
	bool bEquipmentOpen = false;
	
public:
	bool ShouldHideMouse() const;
	void ToggleMouse();
};
