// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "ChatType.generated.h"

UENUM(BlueprintType)
enum class EChatChannel : uint8
{
	World       UMETA(DisplayName="World"),
	Private     UMETA(DisplayName="Private"),
	System      UMETA(DisplayName="System"),
	Team        UMETA(DisplayName="Team"),
	Guild       UMETA(DisplayName="Guild")
};


USTRUCT(BlueprintType)
struct SNTP_API FChatMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SenderName;

	UPROPERTY(BlueprintReadOnly)
	FString ReceiverName;

	UPROPERTY(BlueprintReadOnly)
	FString Content;

	UPROPERTY(BlueprintReadOnly)
	EChatChannel Channel;

	UPROPERTY(BlueprintReadOnly)
	FString Time;
};