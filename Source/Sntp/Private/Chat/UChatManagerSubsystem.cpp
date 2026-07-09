// Fill out your copyright notice in the Description page of Project Settings.


#include "Chat/UChatManagerSubsystem.h"

#include "Chat/ChatComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Player/SntpPlayerState.h"
#include "UObject/FastReferenceCollector.h"

void UUChatManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UUChatManagerSubsystem::HandleWorldMessage(AActor* Sender, const FString& Context)
{
	FChatMessage Message = MakeMessage(Sender, Context, EChatChannel::World);
	TArray<UChatComponent*> Components;
	GetAllChatComponents(Components);

	for (UChatComponent* Component : Components)
	{
		if (Component)
		{
			Component->ClientReceiveChat(Message);
		}
	}
}

void UUChatManagerSubsystem::HandlePrivateMessage(AActor* Sender, const FString& TargetName, const FString& Context)
{
	FChatMessage Message = MakeMessage(Sender, Context, EChatChannel::Private);
	Message.ReceiverName = TargetName;
	TArray<UChatComponent*> Components;
	GetAllChatComponents(Components);
	for (UChatComponent* Component : Components)
	{
		APlayerController* PC = Cast<APlayerController>(Component->GetOwner());
		if (!PC)
		{
			continue;
		}
		
		FString PlayerName  = PC->PlayerState? PC->PlayerState->GetPlayerName() : "";
		
		if (PlayerName == TargetName)
		{
			Component->ClientReceiveChat(Message);
			break;
		}
	}
	UChatComponent* SenderComponent = Sender->FindComponentByClass<UChatComponent>();
	if (SenderComponent)
	{
		SenderComponent->ClientReceiveChat(Message);
	}
}

void UUChatManagerSubsystem::BroadcastSystemMessage(const FString& Content)
{
	FChatMessage Message;
	
	Message.Channel = EChatChannel::System;
	Message.SenderName = TEXT("System");
	Message.Content = Content;
	
	TArray<UChatComponent*> Components;
	GetAllChatComponents(Components);
	
	for (auto Component : Components)
	{
		Component->ClientReceiveChat(Message);
	}
}


void UUChatManagerSubsystem::GetAllChatComponents(TArray<UChatComponent*>& Components)
{
	AGameStateBase* GameState = GetWorld()->GetGameState();
	if (!GameState) return;
	
	for (APlayerState* PS : GameState->PlayerArray)
	{
		APlayerController* PC = Cast<APlayerController>(PS->GetOwner());
		if (!PC) continue;
		
		UChatComponent* Component = PC->FindComponentByClass<UChatComponent>();
		if (Component)
		{
			Components.Add(Component);
		}
	}
}

FChatMessage UUChatManagerSubsystem::MakeMessage(AActor* Sender, const FString& Context, EChatChannel Channel)
{
	FChatMessage Message;
	Message.Channel = Channel;
	Message.Content = Context;
	if (Sender)
	{
		APlayerController* PC = Cast<APlayerController>(Sender);
		if (PC && PC->PlayerState)
		{
			Message.SenderName = PC->PlayerState->GetPlayerName();
		}
	}
	Message.Time = FDateTime::Now().ToString();
	return Message;
}
