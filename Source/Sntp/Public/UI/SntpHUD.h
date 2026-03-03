// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SntpUserWidget.h"
#include "GameFramework/HUD.h"
#include "SntpHUD.generated.h"

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
	
protected:
	virtual void BeginPlay() override;	
private:
	UPROPERTY(EditAnywhere)
	TSubclassOf<USntpUserWidget> OverlayWidgetClass;
};
