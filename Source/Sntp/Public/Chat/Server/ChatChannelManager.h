// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Chat/ChatType.h"

/**
 * Server-side chat channel manager.
 */
class SNTP_API FChatChannelManager
{
public:

	bool CreateChannel(FName ChannelId, EChatChannel ChannelType,
		const FString& DisplayName, bool bAutoDestroy = true);

	bool RemoveChannel(FName ChannelId);

	bool JoinChannel(FName ChannelId, int64 PlayerId);

	bool LeaveChannel(FName ChannelId, int64 PlayerId);

	void LeaveAllChannels(int64 PlayerId);

	bool ContainsChannel(FName ChannelId) const;

	bool ContainsPlayer(FName ChannelId, int64 PlayerId) const;

	FChatChannel* FindChannel(FName ChannelId);
	const FChatChannel* FindChannel(FName ChannelId) const;

	const TSet<int64>* GetMembers(FName ChannelId) const;

	int32 GetMemberCount(FName ChannelId) const;

	int32 GetChannelCount() const;

	void Clear();

private:

	TMap<FName, FChatChannel> ChannelMap;

	TMap<int64, TSet<FName>> PlayerChannelMap;
};

