// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatType.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ChatManagerSubsystem.generated.h"

enum class EChatChannel : uint8;
class UChatComponent;
/**
 * 
 */
UCLASS()
class SNTP_API UChatManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

public:
	/*
		玩家进入服务器
		ChatComponent BeginPlay调用
	*/
	void RegisterPlayer(UChatComponent* ChatComponent);
	
	/*
		玩家离开服务器
	*/
	void UnregisterPlayer(UChatComponent* ChatComponent);

public:
	/*
		公屏聊天
	*/
	void HandleWorldChat(UChatComponent* Sender, const FString& Content);

	/*
		私聊
	*/
	void HandlePrivateChat(UChatComponent* Sender, const FString& TargetName, const FString& Content);
	
	/*
		系统公告
	*/
	void BroadcastSystemMessage(const FString& Content);

private:
	/*
		当前在线玩家聊天组件
		Server Only
	*/
	UPROPERTY()
	TArray<TObjectPtr<UChatComponent>>
	OnlineChatComponents;

private:
	
	/*
		创建消息
	*/
	FChatMessage CreateMessage(UChatComponent* Sender, const FString& Content, EChatChannel Channel);
	
	/*
		查找玩家
	*/
	UChatComponent* FindPlayerByName(const FString& PlayerName);

private:
	
	/*
		敏感词过滤
	*/
	FString FilterMessage(const FString& Content);
	
	/*
		防刷
	*/
	bool CheckChatRate(UChatComponent* Sender);

};
