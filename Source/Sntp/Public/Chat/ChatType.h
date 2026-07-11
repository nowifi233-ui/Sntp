// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "ChatType.generated.h"


UENUM(BlueprintType)
enum class EChatChannel : uint8
{
	World,
	Private,
	System,
	Team,
	Guild
};



USTRUCT(BlueprintType)
struct FChatMessage
{
	GENERATED_BODY()

public:

	//发送者
	UPROPERTY(BlueprintReadOnly)
	FString SenderName;
	
	//接收者
	UPROPERTY(BlueprintReadOnly)
	FString ReceiverName;
	
	//内容
	UPROPERTY(BlueprintReadOnly)
	FString Content;

	UPROPERTY(BlueprintReadOnly)
	EChatChannel Channel;
	
	UPROPERTY(BlueprintReadOnly)
	FDateTime Time;
	

	/*
		消息唯一ID
		后续数据库使用
	*/
	UPROPERTY(BlueprintReadOnly)
	int64 MessageID = 0;
	
};