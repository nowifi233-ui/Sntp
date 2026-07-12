#include "Chat/ChatComponent.h"
#include "Chat/ChatSubsystem.h"
#include "Chat/ChatManagerSubsystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Player/SntpPlayerController.h"
#include "Player/SntpPlayerState.h"

UChatComponent::UChatComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UChatComponent::BeginPlay()
{
    Super::BeginPlay();

    if (GetOwner()->HasAuthority())
    {
        if (UChatManagerSubsystem* ChatManager = GetChatManager())
        {
            ChatManager->RegisterPlayer(this);
        }
    }
    if (GetOwner()->HasAuthority())
    {
        // 每隔 0.1 秒尝试注册，直到成功
        GetWorld()->GetTimerManager().SetTimer(
            RegisterChatTimerHandle,
            this,
            &UChatComponent::TryRegisterChatComponent,
            0.1f,
            true
        );
    }    if (GetOwner()->HasAuthority())
    {
        // 每隔 0.1 秒尝试注册，直到成功
        GetWorld()->GetTimerManager().SetTimer(
            RegisterChatTimerHandle,
            this,
            &UChatComponent::TryRegisterChatComponent,
            0.1f,
            true
        );
    }
}

void UChatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (GetOwner()->HasAuthority())
    {
        if (UChatManagerSubsystem* ChatManager = GetChatManager())
        {
            ChatManager->UnregisterPlayer(this);
        }
    }

    Super::EndPlay(EndPlayReason);
}

void UChatComponent::SendChatMessage(FChatMessage Message)
{
    if (!GetOwner()->HasAuthority())
    {
        ServerSendChatMessage(Message);
        return;
    }

    if (UChatManagerSubsystem* ChatManager = GetChatManager())
    {
        ChatManager->HandleChatMessage(this, Message);
    }
}

void UChatComponent::ServerSendChatMessage_Implementation(FChatMessage Message)
{
    if (UChatManagerSubsystem* ChatManager = GetChatManager())
    {
        ChatManager->HandleChatMessage(this, Message);
    }
}

void UChatComponent::ClientReceiveMessage_Implementation(const FChatMessage& Message)
{
    OnChatMessageReceived.Broadcast(Message);

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UChatSubsystem* ChatSubsystem =
            GameInstance->GetSubsystem<UChatSubsystem>())
        {
            ChatSubsystem->AddMessage(Message);
        }
    }
}

UChatManagerSubsystem* UChatComponent::GetChatManager() const
{
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        return GameInstance->GetSubsystem<UChatManagerSubsystem>();
    }

    return nullptr;
}

void UChatComponent::TryRegisterChatComponent()
{
    ASntpPlayerController* PC = Cast<ASntpPlayerController>(GetOwner());
    // 1. 检查 PlayerState 是否存在
    if (!PC->PlayerState)
        return;

    // 2. 转换为您的自定义 PlayerState（假设 ASntpPlayerState）
    ASntpPlayerState* SntpPS = Cast<ASntpPlayerState>(PC->PlayerState);
    if (!SntpPS) return;

    // 3. 检查 PlayerId 是否已分配（>0 表示有效）
    if (SntpPS->GetPlayerId() == 0) return;

    // 4. 获取 ChatComponent
    UChatComponent* ChatComp = PC->FindComponentByClass<UChatComponent>();
    if (!ChatComp) return;

    // 5. 获取 ChatManagerSubsystem 并注册
    if (UChatManagerSubsystem* ChatManager = PC->GetGameInstance()->GetSubsystem<UChatManagerSubsystem>())
    {
        if (ChatManager->RegisterPlayer(ChatComp))
        {
            // 注册成功，清除定时器
            GetWorld()->GetTimerManager().ClearTimer(RegisterChatTimerHandle);
        }
    }
}
