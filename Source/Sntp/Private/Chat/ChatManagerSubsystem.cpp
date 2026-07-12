#include "Chat/ChatManagerSubsystem.h"
#include "Chat/ChatComponent.h"
#include "Player/SntpPlayerState.h"
#include "GameFramework/PlayerController.h"

void UChatManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SessionManager.Clear();
    ChannelManager.Clear();

    // 默认世界频道
    ChannelManager.CreateChannel(
        TEXT("World"),
        EChatChannel::World,
        TEXT("World"),
        false);

    // 默认系统频道
    ChannelManager.CreateChannel(
        TEXT("System"),
        EChatChannel::System,
        TEXT("System"),
        false);
}

void UChatManagerSubsystem::Deinitialize()
{
    SessionManager.Clear();
    ChannelManager.Clear();

    Super::Deinitialize();
}

bool UChatManagerSubsystem::RegisterPlayer(UChatComponent* ChatComponent)
{
    if (!IsValid(ChatComponent))
    {
        return false;
    }

    ASntpPlayerState* PlayerState = Cast<ASntpPlayerState>(GetPlayerState(ChatComponent));

    if (!PlayerState)
    {
        return false;
    }

    const bool bSuccess = SessionManager.RegisterPlayer(ChatComponent, PlayerState);
    
    if (!bSuccess)
    {
        return false;
    }
    
    const int64 PlayerId = PlayerState->GetPlayerId();
    if (PlayerId != 0)
    {
        ChannelManager.JoinChannel(TEXT("World"), PlayerId);
    }
    return true;
}

void UChatManagerSubsystem::UnregisterPlayer(UChatComponent* ChatComponent)
{
    if (!IsValid(ChatComponent))
    {
        return;
    }

    ASntpPlayerState* PlayerState = Cast<ASntpPlayerState>(GetPlayerState(ChatComponent));
    
    if (PlayerState)
    {
        ChannelManager.LeaveAllChannels(
            PlayerState->GetPlayerId());
    }
    
    SessionManager.UnregisterPlayer(ChatComponent);
}

UChatComponent* UChatManagerSubsystem::GetChatComponent(int64 PlayerId)
{
    return SessionManager.GetChatComponent(PlayerId);
}

const UChatComponent* UChatManagerSubsystem::GetChatComponent(int64 PlayerId) const
{
    return SessionManager.GetChatComponent(PlayerId);
}

APlayerState* UChatManagerSubsystem::GetPlayerState(UChatComponent* ChatComponent) const
{
    if (!IsValid(ChatComponent))
    {
        return nullptr;
    }

    APlayerController* PlayerController =
        Cast<APlayerController>(ChatComponent->GetOwner());

    if (!PlayerController)
    {
        return nullptr;
    }

    return PlayerController->PlayerState;
}

bool UChatManagerSubsystem::FillServerMessage(
    UChatComponent* SenderComponent,
    FChatMessage& Message) const
{
    ASntpPlayerState* PlayerState = Cast<ASntpPlayerState>(GetPlayerState(SenderComponent));


    if (!PlayerState)
    {
        return false;
    }
    
    Message.SenderId = PlayerState->GetPlayerId();
    
    Message.SenderName = PlayerState->GetPlayerName();
    
    Message.Time = FDateTime::UtcNow();
    
    return true;
}

bool UChatManagerSubsystem::HandleChatMessage(
    UChatComponent* SenderComponent,
    FChatMessage& Message)
{
    if (!FillServerMessage(SenderComponent, Message))
    {
        return false;
    }

    if (!ValidateMessage(SenderComponent, Message))
    {
        return false;
    }

    TArray<int64> Receivers;
    RouteMessage(Message, Receivers);

    if (Receivers.IsEmpty())
    {
        return false;
    }

    DispatchMessage(Message, Receivers);

    SessionManager.UpdateLastChatTime(SenderComponent);

    return true;
}

FChatChannelManager& UChatManagerSubsystem::GetChannelManager()
{
    return ChannelManager;
}

const FChatChannelManager& UChatManagerSubsystem::GetChannelManager() const
{
    return ChannelManager;
}

bool UChatManagerSubsystem::ValidateMessage(UChatComponent* SenderComponent, const FChatMessage& Message) const
{
    const FChatSession* Session = SessionManager.FindSession(SenderComponent);
    
    if (!Session)
    {
        return false;
    }
    
    if (!Session->bOnline)
    {
        return false;
    }
    
    if (Session->bMuted)
    {
        return false;
    }

    if (!SessionManager.CheckChatCooldown( SenderComponent, ChatCooldown))
    {
        return false;
    }


    if (Message.Content.TrimStartAndEnd().IsEmpty())
    {
        return false;
    }

    if (Message.Content.Len() > MaxMessageLength)
    {
        return false;
    }

    return true;
}

void UChatManagerSubsystem::RouteMessage(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    OutReceivers.Reset();

    switch (Message.Channel)
    {
    case EChatChannel::World:
        DispatchWorld(Message, OutReceivers);
        break;

    case EChatChannel::Private:
        DispatchPrivate(Message, OutReceivers);
        break;

    case EChatChannel::Team:
        DispatchTeam(Message, OutReceivers);
        break;

    case EChatChannel::Guild:
        DispatchGuild(Message, OutReceivers);
        break;

    case EChatChannel::Nearby:
        DispatchNearby(Message, OutReceivers);
        break;

    case EChatChannel::System:
    case EChatChannel::GM:
        DispatchSystem(Message, OutReceivers);
        break;

    default:
        break;
    }
}

void UChatManagerSubsystem::DispatchMessage(
    const FChatMessage& Message,
    const TArray<int64>& Receivers)
{
    for (int64 PlayerId : Receivers)
    {
        if (UChatComponent* ChatComponent = SessionManager.GetChatComponent(PlayerId))
        {
            ChatComponent->ClientReceiveMessage(Message);
        }
    }
}

void UChatManagerSubsystem::DispatchWorld(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    if (const TSet<int64>* Members = ChannelManager.GetMembers(TEXT("World")))
    {
        OutReceivers.Append(Members->Array());
    }
}

void UChatManagerSubsystem::DispatchPrivate(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    if (SessionManager.IsOnline(GetChatComponent(Message.TargetPlayerId)))
    {
        OutReceivers.Add(Message.TargetPlayerId);
    }

    OutReceivers.AddUnique(Message.SenderId);
}

void UChatManagerSubsystem::DispatchTeam(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    if (const TSet<int64>* Members = ChannelManager.GetMembers(Message.ChannelId))
    {
        OutReceivers.Append(Members->Array());
    }
}

void UChatManagerSubsystem::DispatchGuild(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    if (const TSet<int64>* Members = ChannelManager.GetMembers(Message.ChannelId))
    {
        OutReceivers.Append(Members->Array());
    }
}

void UChatManagerSubsystem::DispatchNearby(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    // TODO:
    // 根据距离筛选玩家
}

void UChatManagerSubsystem::DispatchSystem(
    const FChatMessage& Message,
    TArray<int64>& OutReceivers) const
{
    if (const TSet<int64>* Members = ChannelManager.GetMembers(TEXT("World")))
    {
        OutReceivers.Append(Members->Array());
    }
}