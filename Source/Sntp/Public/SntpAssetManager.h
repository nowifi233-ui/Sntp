// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/AssetManager.h"
#include "SntpAssetManager.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API USntpAssetManager : public UAssetManager
{
	GENERATED_BODY()
public:
	static USntpAssetManager& Get();
	
protected:
	virtual void StartInitialLoading() override;
};
