// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/Server/ChatChannelManager.h"


bool FChatChannelManager::CreateChannel(FName ChannelId,
    EChatChannel ChannelType,
    const FString& DisplayName,
    bool bAutoDestroy)
{
    if (ChannelId.IsNone() || ChannelMap.Contains(ChannelId))
    {
        return false;
    }

    FChatChannel& Channel = ChannelMap.Add(ChannelId);

    Channel.ChannelId = ChannelId;
    Channel.ChannelType = ChannelType;
    Channel.DisplayName = DisplayName;
    Channel.bAutoDestroy = bAutoDestroy;

    return true;
}

bool FChatChannelManager::RemoveChannel(FName ChannelId)
{
    FChatChannel* Channel = ChannelMap.Find(ChannelId);

    if (!Channel)
    {
        return false;
    }

    for (int64 PlayerId : Channel->Members)
    {
        if (TSet<FName>* Channels = PlayerChannelMap.Find(PlayerId))
        {
            Channels->Remove(ChannelId);

            if (Channels->IsEmpty())
            {
                PlayerChannelMap.Remove(PlayerId);
            }
        }
    }

    ChannelMap.Remove(ChannelId);

    return true;
}

bool FChatChannelManager::JoinChannel(FName ChannelId, int64 PlayerId)
{
    FChatChannel* Channel = ChannelMap.Find(ChannelId);

    if (!Channel)
    {
        return false;
    }

    Channel->Members.Add(PlayerId);

    PlayerChannelMap.FindOrAdd(PlayerId).Add(ChannelId);

    return true;
}

bool FChatChannelManager::LeaveChannel(FName ChannelId, int64 PlayerId)
{
    FChatChannel* Channel = ChannelMap.Find(ChannelId);

    if (!Channel)
    {
        return false;
    }

    Channel->Members.Remove(PlayerId);

    if (TSet<FName>* Channels = PlayerChannelMap.Find(PlayerId))
    {
        Channels->Remove(ChannelId);

        if (Channels->IsEmpty())
        {
            PlayerChannelMap.Remove(PlayerId);
        }
    }

    if (Channel->bAutoDestroy && Channel->Members.IsEmpty())
    {
        ChannelMap.Remove(ChannelId);
    }

    return true;
}

void FChatChannelManager::LeaveAllChannels(int64 PlayerId)
{
    TSet<FName>* Channels = PlayerChannelMap.Find(PlayerId);

    if (!Channels)
    {
        return;
    }

    TArray<FName> ChannelIds = Channels->Array();

    for (const FName& ChannelId : ChannelIds)
    {
        LeaveChannel(ChannelId, PlayerId);
    }
}

bool FChatChannelManager::ContainsChannel(FName ChannelId) const
{
    return ChannelMap.Contains(ChannelId);
}

bool FChatChannelManager::ContainsPlayer(FName ChannelId, int64 PlayerId) const
{
    const FChatChannel* Channel = ChannelMap.Find(ChannelId);

    return Channel && Channel->Members.Contains(PlayerId);
}

FChatChannel* FChatChannelManager::FindChannel(FName ChannelId)
{
    return ChannelMap.Find(ChannelId);
}

const FChatChannel* FChatChannelManager::FindChannel(FName ChannelId) const
{
    return ChannelMap.Find(ChannelId);
}

const TSet<int64>* FChatChannelManager::GetMembers(FName ChannelId) const
{
    const FChatChannel* Channel = ChannelMap.Find(ChannelId);

    return Channel ? &Channel->Members : nullptr;
}

int32 FChatChannelManager::GetMemberCount(FName ChannelId) const
{
    const FChatChannel* Channel = ChannelMap.Find(ChannelId);

    return Channel ? Channel->Members.Num() : 0;
}

int32 FChatChannelManager::GetChannelCount() const
{
    return ChannelMap.Num();
}

void FChatChannelManager::Clear()
{
    ChannelMap.Empty();
    PlayerChannelMap.Empty();
}