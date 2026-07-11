// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/ChatSubsystem.h"

void UChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	ChatHistory.Empty();
	UnreadMap.Empty();
	
	// 未读消息
	for (uint8 i = 0; i <= static_cast<uint8>(EChatChannel::Guild); i++)
	{
		UnreadMap.Add(static_cast<EChatChannel>(i), 0);
	}
}

void UChatSubsystem::Deinitialize()
{
	ChatHistory.Empty();
	UnreadMap.Empty();
	
	Super::Deinitialize();
}

void UChatSubsystem::AddMessage(const FChatMessage& Message)
{
	
	/*
		保存历史
	*/
	ChatHistory.Add(Message);
	
	if(ChatHistory.Num() > MaxHistoryCount)
	{
		ChatHistory.RemoveAt(0);
	}

	/*
		未读+1
		UI打开时再清除
	*/

	int32& Count = UnreadMap.FindOrAdd(Message.Channel);
	Count++;
	
	/*
		通知UI
		WBP_Chat监听
	*/
	OnChatMessageAdded.Broadcast(Message);
}

const TArray<FChatMessage>& UChatSubsystem::GetAllMessages() const
{
	return ChatHistory;
}

void UChatSubsystem::GetMessagesByChannel(EChatChannel Channel, TArray<FChatMessage>& OutMessages) const
{
	OutMessages.Empty();
	
	for(const FChatMessage& Msg : ChatHistory)
	{
		if(Msg.Channel == Channel)
		{
			OutMessages.Add(Msg);
		}
	}
}

void UChatSubsystem::ClearHistory()
{
	ChatHistory.Empty();
}

int32 UChatSubsystem::GetUnreadCount(EChatChannel Channel) const
{
	const int32* Count = UnreadMap.Find(Channel);
	
	return Count ? *Count : 0;
}

void UChatSubsystem::ClearUnread(EChatChannel Channel)
{
	UnreadMap.FindOrAdd(Channel) = 0;
}








