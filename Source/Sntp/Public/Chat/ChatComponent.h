// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ChatManagerSubsystem.h"
#include "Components/ActorComponent.h"
#include "ChatType.h"
#include "UChatManagerSubsystem.h"
#include "ChatComponent.generated.h"

struct FChatMessage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnChatMessageReceived,
	FChatMessage,
	Message
);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UChatComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:	
	UPROPERTY(BlueprintAssignable)
	FOnChatMessageReceived OnChatMessageReceived;

public:
	UFUNCTION(BlueprintCallable)
	void SendWorldMessage(const FString& Message);
	
	UFUNCTION(BlueprintCallable)
	void SendPrivateMessage(const FString& TargetPlayer, const FString& Message);
	
	// 服务器->客户端
	UFUNCTION(Client, Reliable)
	void ClientReceiveMessage(const FChatMessage& Message);

private:
	UFUNCTION(Server, Reliable)
	void ServerSendWorldMessage(const FString& Message);
	
	UFUNCTION(Server, Reliable)
	void ServerSendPrivateMessage(const FString& TargetPlayer, const FString& Message);
	
private:
	UChatManagerSubsystem* GetChatManager() const;
};
