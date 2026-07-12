// Fill out your copyright notice in the Description page of Project Settings.


#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Chat/ChatType.h"

#include "ChatComponent.generated.h"

class UChatManagerSubsystem;

/** 收到聊天消息 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage&);

/**
 * Chat network component.
 *
 * 挂载在 PlayerController 上，负责：
 * 1. Client -> Server RPC
 * 2. Server -> Client RPC
 * 3. 注册/注销 ChatManager
 *
 * 不保存聊天记录，不负责 UI。
 */
UCLASS(ClassGroup = (Chat), meta = (BlueprintSpawnableComponent))
class SNTP_API UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	UChatComponent();

protected:

	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:

	/** 客户端发送聊天 */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SendChatMessage(FChatMessage Message);

	/** 收到聊天消息 */
	FOnChatMessageReceived OnChatMessageReceived;

protected:

	/** Client -> Server */
	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(FChatMessage Message);
public:
	/** Server -> Client */
	UFUNCTION(Client, Reliable)
	void ClientReceiveMessage(const FChatMessage& Message);

private:

	UChatManagerSubsystem* GetChatManager() const;
	
	FTimerHandle RegisterChatTimerHandle;
	
	UFUNCTION()
	void TryRegisterChatComponent();
};
