// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InventoryComponent.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "InventoryWidgetController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FInventoryChangedDelegate);

/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SNTP_API UInventoryWidgetController : public USntpWidgetController
{
	GENERATED_BODY()
	
public:
	void Init(UInventoryComponent* InInventoryComponent, APlayerController* InPlayerController);
	void SetTargetInventoryComponent(UInventoryComponent* InInventoryComponent);
	
	UFUNCTION(BlueprintCallable)
	const TArray<FItemInstance>& GetItems() const;
	
	UPROPERTY(BlueprintAssignable)
	FInventoryChangedDelegate OnInventoryChanged;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UInventoryComponent> PlayerInventoryComponent;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UInventoryComponent> TargetInventoryComponent;
	
	UFUNCTION(BlueprintCallable)
	void RequestUseItem(int32 Index);
	
	UFUNCTION(BlueprintCallable)
	void RequestToggleBag();
	
	UFUNCTION(BlueprintCallable)
	void RequestToggleInv();
	
	UFUNCTION(BlueprintCallable)
	void TransferItem(UInventoryComponent* From, UInventoryComponent* To, int32 Index, int32 Count);

	UFUNCTION(BlueprintCallable)
	void HandleShiftClick(UInventoryComponent* From, UInventoryComponent* To, int32 Index, bool IsTarget);
	
	UPROPERTY(BlueprintAssignable)
	FInventoryChangedDelegate EventTargetControllerSet;

private:
	
	UFUNCTION()
	void HandleInventoryChanged();
};
