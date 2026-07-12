// Fill out your copyright notice in the Description page of Project Settings.
#pragma once

#include "CoreMinimal.h"
#include "ChatType.generated.h"

/**
 * 聊天频道
 */
UENUM(BlueprintType)
enum class EChatChannel : uint8
{
    World      UMETA(DisplayName = "World"),
    Private    UMETA(DisplayName = "Private"),
    Team       UMETA(DisplayName = "Team"),
    Guild      UMETA(DisplayName = "Guild"),
    Nearby     UMETA(DisplayName = "Nearby"),
    System     UMETA(DisplayName = "System"),
    GM         UMETA(DisplayName = "GM")
};

/**
 * 聊天消息
 */
USTRUCT(BlueprintType)
struct FChatMessage
{
    GENERATED_BODY()

    /** 发送者ID */
    UPROPERTY(BlueprintReadOnly)
    int64 SenderId = INDEX_NONE;

    /** 接收者ID（私聊使用） */
    UPROPERTY(BlueprintReadOnly)
    int64 TargetPlayerId = INDEX_NONE;

    /** 频道ID（World、Guild_1001、Party_2...） */
    UPROPERTY(BlueprintReadOnly)
    FName ChannelId;

    /** 频道类型 */
    UPROPERTY(BlueprintReadOnly)
    EChatChannel Channel = EChatChannel::World;

    /** 发送者名字 */
    UPROPERTY(BlueprintReadOnly)
    FString SenderName;

    /** 聊天内容 */
    UPROPERTY(BlueprintReadOnly)
    FString Content;

    /** 时间 */
    UPROPERTY(BlueprintReadOnly)
    FDateTime Time;
};

/**
 * 玩家聊天会话
 */
USTRUCT()
struct FChatSession
{
    GENERATED_BODY()

    /** 玩家唯一ID */
    UPROPERTY()
    int64 PlayerId = INDEX_NONE;

    /** 玩家昵称 */
    UPROPERTY()
    FString PlayerName;

    /** 是否禁言 */
    UPROPERTY()
    bool bMuted = false;

    /** 上一次发言时间 */
    double LastChatTime = 0.0;

    /** 登录时间 */
    FDateTime LoginTime;

    /** 当前加入的频道 */
    TSet<FName> JoinedChannels;

    /** ChatComponent */
    TWeakObjectPtr<class UChatComponent> ChatComponent;

    /** PlayerState */
    TWeakObjectPtr<class APlayerState> PlayerState;

    bool IsValid() const
    {
        return PlayerId != INDEX_NONE;
    }
    
    UPROPERTY()
    bool bOnline = false;
};

/**
 * 聊天频道
 */
struct FChatChannel
{
    /** ChannelId */
    FName ChannelId;

    /** 世界/公会/队伍 */
    EChatChannel ChannelType = EChatChannel::World;

    /** 显示名字 */
    FString DisplayName;

    /** 玩家列表 */
    TSet<int64> Members;

    /** 空频道自动删除 */
    bool bAutoDestroy = true;

    bool IsEmpty() const
    {
        return Members.IsEmpty();
    }

    int32 Num() const
    {
        return Members.Num();
    }
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&);

