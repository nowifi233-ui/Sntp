// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/InventoryComponent.h"
#include "UI/WidgetController/InventoryWidgetController.h"
#include "UI/Widgets/SntpUserWidget.h"
#include "SntpInventoryWidget.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API USntpInventoryWidget : public USntpUserWidget
{
	GENERATED_BODY()
	
public:
	void InitComponents(UInventoryComponent* InActorComponent, UInventoryComponent* InPlayerComponent);
	
	UPROPERTY(BlueprintReadOnly)
	UInventoryComponent* ActorInventoryComponent;
	
	UPROPERTY(BlueprintReadOnly)
	UInventoryComponent* PlayerInventoryComponent;
};
