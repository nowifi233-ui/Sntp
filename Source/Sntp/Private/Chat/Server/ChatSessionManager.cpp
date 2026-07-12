#include "Chat/Server/ChatSessionManager.h"

#include "Chat/ChatComponent.h"
#include "GameFramework/PlayerState.h"


bool FChatSessionManager::RegisterPlayer(UChatComponent* ChatComponent, APlayerState* PlayerState)
{
	if (!ChatComponent || !PlayerState)
	{
		return false;
	}

	const int64 PlayerId = PlayerState->GetPlayerId();

	if (PlayerId == INDEX_NONE)
	{
		return false;
	}

	FChatSession& Session = SessionMap.FindOrAdd(ChatComponent);

	Session.PlayerId = PlayerId;
	Session.PlayerName = PlayerState->GetPlayerName();
	Session.ChatComponent = ChatComponent;
	Session.PlayerState = PlayerState;
	Session.LoginTime = FDateTime::UtcNow();
	Session.LastChatTime = 0.f;
	Session.bOnline = true;
	Session.bMuted = false;


	PlayerIdMap.Add(PlayerId, ChatComponent);

	PlayerNameMap.Add(Session.PlayerName, ChatComponent);
	
	return true;
}


bool FChatSessionManager::UnregisterPlayer(
	UChatComponent* ChatComponent)
{
	if (!ChatComponent)
	{
		return false;
	}


	FChatSession* Session = SessionMap.Find(ChatComponent);

	if (!Session)
	{
		return false;
	}


	PlayerIdMap.Remove(Session->PlayerId);

	PlayerNameMap.Remove(Session->PlayerName);


	Session->bOnline = false;

	Session->ChatComponent.Reset();


	SessionMap.Remove(ChatComponent);


	return true;
}


FChatSession* FChatSessionManager::FindSession(
	UChatComponent* ChatComponent)
{
	return SessionMap.Find(ChatComponent);
}


const FChatSession* FChatSessionManager::FindSession(
	UChatComponent* ChatComponent) const
{
	return SessionMap.Find(ChatComponent);
}


FChatSession* FChatSessionManager::FindSession(
	int64 PlayerId)
{
	const TWeakObjectPtr<UChatComponent>* Component =
		PlayerIdMap.Find(PlayerId);


	if (!Component || !Component->IsValid())
	{
		return nullptr;
	}


	return SessionMap.Find(Component->Get());
}


const FChatSession* FChatSessionManager::FindSession(
	int64 PlayerId) const
{
	const TWeakObjectPtr<UChatComponent>* Component =
		PlayerIdMap.Find(PlayerId);


	if (!Component || !Component->IsValid())
	{
		return nullptr;
	}


	return SessionMap.Find(Component->Get());
}


FChatSession* FChatSessionManager::FindSession(
	const FString& PlayerName)
{
	const TWeakObjectPtr<UChatComponent>* Component =
		PlayerNameMap.Find(PlayerName);


	if (!Component || !Component->IsValid())
	{
		return nullptr;
	}


	return SessionMap.Find(Component->Get());
}


const FChatSession* FChatSessionManager::FindSession(
	const FString& PlayerName) const
{
	const TWeakObjectPtr<UChatComponent>* Component =
		PlayerNameMap.Find(PlayerName);


	if (!Component || !Component->IsValid())
	{
		return nullptr;
	}


	return SessionMap.Find(Component->Get());
}


bool FChatSessionManager::Contains(
	UChatComponent* ChatComponent) const
{
	return SessionMap.Contains(ChatComponent);
}


bool FChatSessionManager::IsOnline(const UChatComponent* ChatComponent) const
{
	const FChatSession* Session = SessionMap.Find(ChatComponent);


	return Session && Session->bOnline;
}


bool FChatSessionManager::SetMuted(
	UChatComponent* ChatComponent,
	bool bMuted)
{
	FChatSession* Session =
		SessionMap.Find(ChatComponent);


	if (!Session)
	{
		return false;
	}


	Session->bMuted = bMuted;

	return true;
}


bool FChatSessionManager::IsMuted(
	UChatComponent* ChatComponent) const
{
	const FChatSession* Session =
		SessionMap.Find(ChatComponent);


	return Session && Session->bMuted;
}


void FChatSessionManager::UpdateLastChatTime(UChatComponent* ChatComponent)
{
	FChatSession* Session = SessionMap.Find(ChatComponent);


	if (Session)
	{
		Session->LastChatTime = FPlatformTime::Seconds();
	}
}


bool FChatSessionManager::CheckChatCooldown(
	UChatComponent* ChatComponent,
	double Cooldown) const
{
	const FChatSession* Session =
		SessionMap.Find(ChatComponent);


	if (!Session)
	{
		return false;
	}


	return FPlatformTime::Seconds() - Session->LastChatTime >= Cooldown;
}


UChatComponent* FChatSessionManager::GetChatComponent(int64 PlayerId)
{
	const TWeakObjectPtr<UChatComponent>* Component =
		PlayerIdMap.Find(PlayerId);


	return Component ? Component->Get() : nullptr;
}

const UChatComponent* FChatSessionManager::GetChatComponent(int64 PlayerId) const
{
	const TWeakObjectPtr<UChatComponent>* Component = PlayerIdMap.Find(PlayerId);
	return Component ? Component->Get() : nullptr;
}


int32 FChatSessionManager::GetPlayerCount() const
{
	return SessionMap.Num();
}


void FChatSessionManager::Clear()
{
	SessionMap.Empty();

	PlayerIdMap.Empty();

	PlayerNameMap.Empty();
}