// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/Widgets/SntpUserWidget.h"
#include "GameFramework/HUD.h"
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
	
private:
	bool bBagOpen = false;
};
