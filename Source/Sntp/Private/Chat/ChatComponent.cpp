// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/ChatComponent.h"

// Sets default values for this component's properties
UChatComponent::UChatComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}


// Called when the game starts
void UChatComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UChatComponent::SendWorldMessage(const FString& Message)
{
	if (Message.IsEmpty()) return;
	ServerSendWorldMessage(Message);
}

void UChatComponent::SendPrivateMessage(const FString& TargetPlayer, const FString& Message)
{
	if (Message.IsEmpty()) return;
	if (TargetPlayer.IsEmpty()) return;
	ServerSendPrivateMessage(TargetPlayer, Message);
	
}

UUChatManagerSubsystem* UChatComponent::GetChatManager() const
{
	return GetWorld()->GetSubsystem<UUChatManagerSubsystem>();
}

void UChatComponent::ClientReceiveChat_Implementation(const FChatMessage& Message)
{
	/*
	 * 通知 UI
	 */
	OnChatMessageReceived.Broadcast(Message);
}

void UChatComponent::ServerSendWorldMessage_Implementation(const FString& Message)
{
	UUChatManagerSubsystem* Manager = GetWorld()->GetSubsystem<UUChatManagerSubsystem>();
	if (Manager)
	{
		Manager->HandleWorldMessage(GetOwner(), Message);
	}
}

void UChatComponent::ServerSendPrivateMessage_Implementation(const FString& TargetPlayer, const FString& Message)
{
	UUChatManagerSubsystem* Manager = GetWorld()->GetSubsystem<UUChatManagerSubsystem>();
	if (Manager)
	{
		Manager->HandlePrivateMessage(GetOwner(), TargetPlayer, Message);
	}
}




