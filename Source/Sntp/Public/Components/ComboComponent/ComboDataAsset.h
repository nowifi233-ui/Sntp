// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ComboDataAsset.generated.h"

class UGameplayAbility;

UENUM(BlueprintType)
enum class EInputType : uint8
{
	None,
	Attack,
	Heavy,
	Skill
};

USTRUCT(BlueprintType)
struct FComboNode
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName NodeID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<UGameplayAbility> Ability;
	
	UPROPERTY(EditAnywhere)
	TMap<EInputType, FName> NextNodes;
	
	UPROPERTY(EditAnywhere)
	float InputWindow = 0.5f;
};

USTRUCT(BlueprintType)
struct FPendingInput
{
	GENERATED_BODY()
	
	EInputType InputType = EInputType::None;
	float TimeStamp = 0.f;
	float ValidTime = 0.3f;
	bool bIsValid = false;
	void Clear()
	{
		InputType = EInputType::None;
		bIsValid = false;
	}
};
/**
 * 
 */
UCLASS()
class SNTP_API UComboDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	FName StartNode;

	UPROPERTY(EditAnywhere)
	TMap<FName, FComboNode> Nodes;
};
