// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/ChatComponent.h"

#include "Chat/ChatManagerSubsystem.h"
#include "Chat/ChatSubsystem.h"

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
	
	if(GetOwner()->HasAuthority())
	{
		UChatManagerSubsystem* Manager = GetChatManager();
		
		if(Manager)
		{
			Manager->RegisterPlayer(this);
		}
	}

}

void UChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if(GetOwner()->HasAuthority())
	{
		UChatManagerSubsystem* Manager = GetChatManager();
		
		if(Manager)
		{
			Manager->UnregisterPlayer(this);
		}
	}
	Super::EndPlay(EndPlayReason);
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

UChatManagerSubsystem* UChatComponent::GetChatManager() const
{
	if(!GetOwner()->GetGameInstance()) return nullptr;
	
	return GetOwner()->GetGameInstance()->GetSubsystem<UChatManagerSubsystem>();
}

void UChatComponent::ClientReceiveMessage_Implementation(const FChatMessage& Message)
{
	/*
	 * 通知 UI
	 */
	UChatSubsystem* ChatSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UChatSubsystem>();
	if (ChatSubsystem)
	{
		ChatSubsystem->AddMessage(Message);
	}
}

void UChatComponent::ServerSendWorldMessage_Implementation(const FString& Message)
{
	UChatManagerSubsystem* Manager = GetChatManager();
	if (Manager)
	{
		Manager->HandleWorldChat(this, Message);
	}
}

void UChatComponent::ServerSendPrivateMessage_Implementation(const FString& TargetPlayer, const FString& Message)
{
	UChatManagerSubsystem* Manager = GetChatManager();
	if (Manager)
	{
		Manager->HandlePrivateChat(this, TargetPlayer, Message);
	}
}




