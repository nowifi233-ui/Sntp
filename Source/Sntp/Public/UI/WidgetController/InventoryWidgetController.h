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
	
	UFUNCTION(BlueprintCallable)
	const TArray<FItemInstance>& GetItems() const;
	
	UPROPERTY(BlueprintAssignable)
	FInventoryChangedDelegate OnInventoryChanged;
	
	UPROPERTY(BlueprintReadOnly)
	TWeakObjectPtr<UInventoryComponent> InventoryComponent;
	
	UFUNCTION(BlueprintCallable)
	void RequestUseItem(int32 Index);
	
	UFUNCTION(BlueprintCallable)
	void RequestToggleBag();
private:
	
	
	UFUNCTION()
	void HandleInventoryChanged();
};
