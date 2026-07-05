// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/EquipmentComponent/EquipmentComponent.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "EquipmentWidgetController.generated.h"

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class SNTP_API UEquipmentWidgetController : public USntpWidgetController
{
	GENERATED_BODY()
public:
	void Init(UEquipmentComponent* InEquipmentComponent, APlayerController* InPlayerController);

	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<UEquipmentComponent> PlayerEquipmentComponent;
	
};
