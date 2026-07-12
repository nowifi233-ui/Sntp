#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"

#include "Chat/ChatType.h"

#include "ChatSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnChatMessageAdded, const FChatMessage&, ChatMessage);

/**
 * Client chat subsystem.
 *
 * 职责：
 * - 保存聊天记录
 * - 管理各频道消息
 * - 管理私聊会话
 * - 提供查询接口
 *
 * 不负责网络通信。
 */
UCLASS()
class SNTP_API UChatSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	virtual void Deinitialize() override;

public:

	/** 收到聊天消息 */
	void HandleReceiveMessage(const FChatMessage& Message);

	/** 添加聊天消息 */
	void AddMessage(const FChatMessage& Message);

	/** 清空聊天记录 */
	void ClearHistory();

	/** 获取全部聊天记录 */
	const TArray<FChatMessage>& GetHistory() const;

	/** 获取频道聊天记录 */
	void GetChannelHistory(EChatChannel Channel, TArray<FChatMessage>& OutMessages) const;

public:

	/** UI监听 */
	UPROPERTY(BlueprintAssignable)
	FOnChatMessageAdded OnChatMessageAdded;

private:

	/** 全部聊天记录 */
	TArray<FChatMessage> ChatHistory;

	/** 每个频道历史 */
	TMap<EChatChannel, TArray<FChatMessage>> ChannelHistory;
	
public:
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SendMessage(
		const FString& Content,
		EChatChannel Channel = EChatChannel::World,
		int64 TargetPlayerId = -1);
};