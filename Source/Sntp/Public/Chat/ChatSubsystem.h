// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatType.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChatSubsystem.generated.h"

/**
 * 
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnChatMessageAdded,
	const FChatMessage&,
	Message
);

UCLASS()
class SNTP_API UChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

public:
	
	/*
		添加服务器消息
	*/
	void AddMessage(const FChatMessage& Message);

public:

	/*
		获取全部聊天
	*/
	const TArray<FChatMessage>& GetAllMessages() const;
	
	/*
		获取指定频道
	*/
	void GetMessagesByChannel(EChatChannel Channel, TArray<FChatMessage>& OutMessages) const;

public:

	/*
		清空
	*/
	void ClearHistory();

public:

	/*
		未读消息
	*/
	int32 GetUnreadCount(EChatChannel Channel) const;

	void ClearUnread(EChatChannel Channel);
public:
	
	UPROPERTY(BlueprintAssignable)
	FOnChatMessageAdded OnChatMessageAdded;

private:
	
	/*
		全部历史
		最近100条
	*/
	UPROPERTY()
	TArray<FChatMessage> ChatHistory;
	
	/*
		未读数量
	*/
	TMap<EChatChannel,int32> UnreadMap;
private:
	
	int32 MaxHistoryCount = 100;
};
