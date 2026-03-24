// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GraphicsSettings.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API UGraphicsSettings : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	
	UFUNCTION(blueprintCallable)
	void SetResolution(int32 Width, int32 Height);
	
	UFUNCTION(BlueprintCallable)
	void SetFullscreenMode(EWindowMode::Type Mode);
	
	UFUNCTION(BlueprintCallable)
	void SetGraphicsQuality(int32 Level);
	
	UFUNCTION(BlueprintCallable)
	void ApplySettings();
	
private:
	class UGameUserSettings* Settings;
};
