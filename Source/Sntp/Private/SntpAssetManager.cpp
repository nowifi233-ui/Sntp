// Fill out your copyright notice in the Description page of Project Settings.


#include "SntpAssetManager.h"
#include "SntpGameplayTags.h"

USntpAssetManager& USntpAssetManager::Get()
{
	check(GEngine)
	USntpAssetManager* SntpAssetManager = Cast<USntpAssetManager>(GEngine->AssetManager);
	return *SntpAssetManager;
}

void USntpAssetManager::StartInitialLoading()
{
	Super::StartInitialLoading();
	FSntpGameplayTags::InitializeNativeGameplayTags();
	
}
