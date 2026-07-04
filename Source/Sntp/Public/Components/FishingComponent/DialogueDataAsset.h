// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "DialogueDataAsset.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FDialogueChoice
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText ChoiceText;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName NextNodeID;
};

USTRUCT(BlueprintType, Blueprintable)
struct FDialogueNode
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FName NodeID;
	
	UPROPERTY(EditAnywhere)
	FName NextID;
	
	UPROPERTY(EditAnywhere)
	FText Speaker;

	UPROPERTY(EditAnywhere)
	FText Content;

	UPROPERTY(EditAnywhere)
	TArray<FDialogueChoice> Choices;
	
	bool operator==(const FDialogueNode& Other) const
	{
		return NodeID == Other.NodeID;
	}
};

/**
 * 
 */
UCLASS()
class SNTP_API UDialogueDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere)
	TArray<FDialogueNode> Nodes;
	
	UFUNCTION(BlueprintCallable)
	FDialogueNode GetNodeByID(FName NodeID);
};
