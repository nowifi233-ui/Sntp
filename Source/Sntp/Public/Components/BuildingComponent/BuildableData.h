// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "BuildableData.generated.h"

struct FInstancedStruct;
class UItemDefinition;
/**
 * 
 */
USTRUCT()
struct SNTP_API FBuildableData : public FTableRowBase
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly)
	FGameplayTag Tag;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* StaticMesh;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> Class;
	
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ETraceTypeQuery> TraceChannel;
	
	UPROPERTY(EditDefaultsOnly)
	UItemDefinition* ItemDefinition;
	
	UPROPERTY(EditDefaultsOnly)
	TMap<FGameplayTag, float> Attributes;
	
	UPROPERTY(EditDefaultsOnly)
	FInstancedStruct CustomData;
};
