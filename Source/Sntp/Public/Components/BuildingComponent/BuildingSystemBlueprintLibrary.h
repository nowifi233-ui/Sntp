// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BuildingSystemBlueprintLibrary.generated.h"

struct FGameplayTag;
struct FBuildableData;
/**
 * 
 */
UCLASS()
class SNTP_API UBuildingSystemBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	UFUNCTION(BlueprintCallable)
	static UDataTable* GetBuildableDataTable();
	
	UFUNCTION(BlueprintCallable)
	static FBuildableData GetBuildableDataByTag(FGameplayTag Tag);
	
	UFUNCTION(BlueprintCallable)
	static UStaticMesh* GetBuildableMeshByTag(FGameplayTag Tag);
	
	UFUNCTION(BlueprintCallable)
	static FGameplayTagContainer GetAllBuildableTag();
	
	
	
};
