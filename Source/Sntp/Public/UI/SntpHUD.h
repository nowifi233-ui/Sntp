// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/SntpUserWidget.h"
#include "GameFramework/HUD.h"
#include "WidgetController/CraftingWidgetController.h"
#include "WidgetController/InventoryWidgetController.h"
#include "SntpHUD.generated.h"

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
	 * 
	 */
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUserWidget> SettingWidgetClass;
	
	UPROPERTY()
	TObjectPtr<UUserWidget> SettingWidget;
	
private:
	bool bBagOpen = false;
	bool bSettingOpen = false;
	bool bInventoryOpen = false;
	bool bCraftingOpen = false;
	
	bool ShouldHideMouse() const;
	void ToggleMouse();
};
