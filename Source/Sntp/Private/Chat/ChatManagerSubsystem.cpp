// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/ChatManagerSubsystem.h"

#include "Chat/ChatComponent.h"
#include "Chat/ChatType.h"
#include "GameFramework/PlayerState.h"

void UChatManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	OnlineChatComponents.Empty();
}

void UChatManagerSubsystem::RegisterPlayer(UChatComponent* ChatComponent)
{
	if(!ChatComponent) return;
	
	if(!OnlineChatComponents.Contains(ChatComponent))
	{
		OnlineChatComponents.Add(ChatComponent);
	}
}

void UChatManagerSubsystem::UnregisterPlayer(UChatComponent* ChatComponent)
{
	OnlineChatComponents.Remove(ChatComponent);
}

void UChatManagerSubsystem::HandleWorldChat(UChatComponent* Sender, const FString& Content)
{
	if(!CheckChatRate(Sender)) return;
	
	FString Filtered = FilterMessage(Content);
	
	FChatMessage Message = CreateMessage(Sender, Filtered, EChatChannel::World);
	
	for(UChatComponent* Component : OnlineChatComponents)
	{
		if(Component)
		{
			Component->ClientReceiveMessage(Message);
		}
	}
}

void UChatManagerSubsystem::HandlePrivateChat(UChatComponent* Sender, const FString& TargetName, const FString& Content)
{
	if(!Sender) return;
	FString Filtered = FilterMessage(Content);
	FChatMessage Message = CreateMessage(Sender, Filtered, EChatChannel::Private);
	// 对方显示
	UChatComponent* Target = FindPlayerByName(TargetName);
	if (Target)
	{
		Target->ClientReceiveMessage(Message);
	}
	// 自己显示
	Sender->ClientReceiveMessage(Message);
}

void UChatManagerSubsystem::BroadcastSystemMessage(const FString& Content)
{
	FChatMessage Message;
	Message.Channel = EChatChannel::System;
	Message.SenderName = "System";
	Message.Content = Content;
	for (UChatComponent* Component : OnlineChatComponents)
	{
		if (Component)
		{
			Component->ClientReceiveMessage(Message);
		}
	}
}

FChatMessage UChatManagerSubsystem::CreateMessage(UChatComponent* Sender, const FString& Content, EChatChannel Channel)
{
	FChatMessage Message;
	Message.Channel = Channel;
	Message.Content = Content;
	Message.Time = FDateTime::Now();
	
	if(Sender)
	{
		APlayerController* PC = Cast<APlayerController>(Sender->GetOwner());
		
		if(PC && PC->PlayerState)
		{
			Message.SenderName = PC->PlayerState->GetPlayerName();
		}
	}
	return Message;
}

UChatComponent* UChatManagerSubsystem::FindPlayerByName(const FString& PlayerName)
{
	for(UChatComponent* Component : OnlineChatComponents)
	{
		APlayerController* PC = Cast<APlayerController>(Component->GetOwner());
		
		if(!PC) continue;
		if(PC->PlayerState && PC->PlayerState->GetPlayerName() == PlayerName)
		{
			return Component;
		}
	}
	return nullptr;
}

FString UChatManagerSubsystem::FilterMessage(const FString& Content)
{
	FString Result = Content;

	TArray<FString> BlockWords =
	{
		TEXT("xxx"),
		TEXT("bad")
	};
	
	for(auto& Word : BlockWords)
	{
		Result.ReplaceInline(*Word, TEXT("***"));
	}
	
	return Result;
}

bool UChatManagerSubsystem::CheckChatRate(UChatComponent* Sender)
{
	/*
		TODO:

		TMap<PlayerID,LastChatTime>

		例如:
		2秒一次
	*/
	
	return true;
}










