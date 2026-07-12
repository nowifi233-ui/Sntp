#pragma once

#include "CoreMinimal.h"
#include "Chat/ChatType.h"

class UChatComponent;
class APlayerState;

/**
 * Server-side chat session manager.
 */
class SNTP_API FChatSessionManager
{
public:

	bool RegisterPlayer(UChatComponent* ChatComponent, APlayerState* PlayerState);

	bool UnregisterPlayer(UChatComponent* ChatComponent);

	FChatSession* FindSession(UChatComponent* ChatComponent);
	const FChatSession* FindSession(UChatComponent* ChatComponent) const;

	FChatSession* FindSession(int64 PlayerId);
	const FChatSession* FindSession(int64 PlayerId) const;

	FChatSession* FindSession(const FString& PlayerName);
	const FChatSession* FindSession(const FString& PlayerName) const;

	bool Contains(UChatComponent* ChatComponent) const;

	bool IsOnline(const UChatComponent* ChatComponent) const;

	bool SetMuted(UChatComponent* ChatComponent, bool bMuted);

	bool IsMuted(UChatComponent* ChatComponent) const;

	void UpdateLastChatTime(UChatComponent* ChatComponent);

	bool CheckChatCooldown(UChatComponent* ChatComponent, double Cooldown) const;

	UChatComponent* GetChatComponent(int64 PlayerId);
	const UChatComponent* GetChatComponent(int64 PlayerId) const;

	int32 GetPlayerCount() const;

	void Clear();

private:

	/** ChatComponent -> Session */
	TMap<TWeakObjectPtr<UChatComponent>, FChatSession> SessionMap;

	/** PlayerId -> ChatComponent */
	TMap<int64, TWeakObjectPtr<UChatComponent>> PlayerIdMap;

	/** PlayerName -> ChatComponent */
	TMap<FString, TWeakObjectPtr<UChatComponent>> PlayerNameMap;
};