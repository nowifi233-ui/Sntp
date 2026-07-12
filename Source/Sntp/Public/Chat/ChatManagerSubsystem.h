#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Chat/ChatType.h"
#include "Chat/Server/ChatSessionManager.h"
#include "Chat/Server/ChatChannelManager.h"
#include "ChatManagerSubsystem.generated.h"

class UChatComponent;
class APlayerController;
class APlayerState;

/**
 * Server-side chat manager.
 *
 * Responsibilities:
 * - Register / Unregister players
 * - Validate chat messages
 * - Resolve receivers
 * - Dispatch messages
 *
 * Does NOT own UI or chat history.
 */
UCLASS()
class SNTP_API UChatManagerSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:

    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    //~ End USubsystem Interface

public:

    /** 注册聊天组件 */
    bool RegisterPlayer(UChatComponent* ChatComponent);

    /** 注销聊天组件 */
    void UnregisterPlayer(UChatComponent* ChatComponent);

    /** 处理客户端发送的聊天消息 */
    bool HandleChatMessage(UChatComponent* SenderComponent, FChatMessage& Message);

    /** 获取玩家聊天组件 */
    UChatComponent* GetChatComponent(int64 PlayerId);
    
    const UChatComponent* GetChatComponent(int64 PlayerId) const;

    /** Session Manager */
    FORCEINLINE FChatSessionManager& GetSessionManager()
    {
        return SessionManager;
    }

    const FChatSessionManager& GetSessionManager() const
    {
        return SessionManager;
    }

    /** Channel Manager */
    FChatChannelManager& GetChannelManager();

    const FChatChannelManager& GetChannelManager() const;

private:

    /** 检查消息是否合法 */
    bool ValidateMessage(UChatComponent* SenderComponent, const FChatMessage& Message) const;

    /** 根据频道计算接收者 */
    void RouteMessage(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 向目标玩家发送消息 */
    void DispatchMessage(const FChatMessage& Message, const TArray<int64>& Receivers);

    /** 世界频道 */
    void DispatchWorld(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 私聊 */
    void DispatchPrivate(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 队伍频道 */
    void DispatchTeam(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 公会频道 */
    void DispatchGuild(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 附近频道 */
    void DispatchNearby(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 系统频道 */
    void DispatchSystem(const FChatMessage& Message, TArray<int64>& OutReceivers) const;

    /** 从 ChatComponent 获取 PlayerState */
    APlayerState* GetPlayerState(UChatComponent* ChatComponent) const;

    /** 填充服务器消息信息 */
    bool FillServerMessage(UChatComponent* SenderComponent, FChatMessage& Message) const;

private:

    /** 在线玩家管理 */
    FChatSessionManager SessionManager;

    /** 聊天频道管理 */
    FChatChannelManager ChannelManager;

    /** 最大发言长度 */
    UPROPERTY()
    int32 MaxMessageLength = 200;

    /** 发言冷却（秒） */
    UPROPERTY()
    float ChatCooldown = 0.5f;
};