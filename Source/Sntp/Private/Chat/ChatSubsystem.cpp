#include "Chat/ChatSubsystem.h"

#include "Chat/ChatComponent.h"

void UChatSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	ChatHistory.Reset();
	ChannelHistory.Reset();
}

void UChatSubsystem::Deinitialize()
{
	ChatHistory.Reset();
	ChannelHistory.Reset();

	Super::Deinitialize();
}

void UChatSubsystem::HandleReceiveMessage(const FChatMessage& Message)
{
	AddMessage(Message);
}

void UChatSubsystem::AddMessage(const FChatMessage& Message)
{
	ChatHistory.Add(Message);

	ChannelHistory.FindOrAdd(Message.Channel).Add(Message);

	OnChatMessageAdded.Broadcast(Message);
}

void UChatSubsystem::ClearHistory()
{
	ChatHistory.Reset();
	ChannelHistory.Reset();
}

const TArray<FChatMessage>& UChatSubsystem::GetHistory() const
{
	return ChatHistory;
}

void UChatSubsystem::GetChannelHistory(
	EChatChannel Channel,
	TArray<FChatMessage>& OutMessages) const
{
	OutMessages.Reset();

	if (const TArray<FChatMessage>* Messages = ChannelHistory.Find(Channel))
	{
		OutMessages = *Messages;
	}
}

void UChatSubsystem::SendMessage(const FString& Content, EChatChannel Channel, int64 TargetPlayerId)
{
	if (Content.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC)
	{
		return;
	}

	UChatComponent* ChatComponent = PC->FindComponentByClass<UChatComponent>();

	if (!ChatComponent)
	{
		return;
	}

	FChatMessage Message;
	Message.Channel = Channel;
	Message.TargetPlayerId = TargetPlayerId;
	Message.Content = Content;

	ChatComponent->SendChatMessage(Message);
}

