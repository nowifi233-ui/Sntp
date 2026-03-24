// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Widgets/GraphicsSettings.h"

#include "GameFramework/GameUserSettings.h"

void UGraphicsSettings::NativeConstruct()
{
	Super::NativeConstruct();
	
	Settings = GEngine->GetGameUserSettings();
}

void UGraphicsSettings::SetResolution(int32 Width, int32 Height)
{
	if (Settings)
	{
		Settings->SetScreenResolution(FIntPoint(Width, Height));
	}
}

void UGraphicsSettings::SetFullscreenMode(EWindowMode::Type Mode)
{
	if (Settings)
	{
		Settings->SetFullscreenMode(Mode);
	}
}

void UGraphicsSettings::SetGraphicsQuality(int32 Level)
{
	if (Settings)
	{
		Settings->SetOverallScalabilityLevel(Level);
	}
}

void UGraphicsSettings::ApplySettings()
{
	if (Settings)
	{
		Settings->ApplySettings(false);
		Settings->SaveSettings();
	}
}
