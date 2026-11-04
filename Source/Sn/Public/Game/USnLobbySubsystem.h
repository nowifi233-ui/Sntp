// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "FindSessionsCallbackProxy.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "USnLobbySubsystem.generated.h"

class FOnlineSessionSearch;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnLobbyCreatedDelegate, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FSnLobbiesFoundDelegate, const TArray<FBlueprintSessionResult>&, Results, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnLobbyJoinedDelegate, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnLobbyDestroyedDelegate, bool, bSuccess);

/**
 * LAN lobby layer over the engine's OnlineSubsystemNull (the standard
 * community setup for Risk-of-Rain-style LAN co-op): create a room, list
 * rooms on the local network, join one. Session discovery is the Null
 * subsystem's LAN beacon (UDP broadcast); the actual connection runs over the
 * default IP net driver once the join travels to the host's address.
 *
 * Manual IOnlineSession calls (not the blueprint proxies): the proxies cannot
 * attach custom session settings, and the lobby list needs the host's player
 * ID as a display name ("HostName" setting key).
 *
 * Gameplay flow:
 *   Host:  CreateLobby(ID, MaxPlayers) -> OnLobbyCreated(true) -> the menu
 *          calls USnGameInstance::StartNewGame (travels with ?listen).
 *   Guest: FindLobbies() -> OnLobbiesFound -> JoinLobby(Result) -> on success
 *          the engine travels to the host; the controller reports the ID via
 *          ServerSetPlayerName once possessed (JoinSession carries no name).
 *   Both:  the host's session is the source of truth for the run - guests
 *          never write their own saves (RoR2 run semantics).
 */
UCLASS()
class SN_API USnLobbySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End of USubsystem interface

	/** Host: creates a LAN session advertising HostName (the player ID) and
	 *  MaxPlayers. Results in OnLobbyCreated; travel afterwards (StartNewGame
	 *  with ?listen) only makes sense once it succeeded. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Lobby")
	void CreateLobby(int32 MaxPlayers, const FString& HostName);

	/** Guest: broadcasts a LAN search. Results in OnLobbiesFound. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Lobby")
	void FindLobbies();

	/** Guest: joins a room from the search results. On success the engine
	 *  travels to the host automatically (OnLobbyJoined(true)). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Lobby")
	void JoinLobby(const FBlueprintSessionResult& LobbyResult);

	/** Leaves/destroys the current session (host dissolves the room, guest
	 *  disconnects). No-op when not in a session. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Lobby")
	void DestroyLobby();

	/** Room display name for the lobby list: the host's advertised player ID,
	 *  falling back to the owning user name. */
	UFUNCTION(BlueprintPure, Category = "Sn|Lobby")
	static FString GetLobbyDisplayName(const FBlueprintSessionResult& LobbyResult);

	/** True once this machine created or joined a session (host or member). */
	bool IsInLobby() const { return bIsInLobby; }

	/** True when this machine created the session (travels with ?listen). */
	bool IsHosting() const { return bIsHosting; }

	/** Created (host) - the menu then starts the game. */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Lobby")
	FSnLobbyCreatedDelegate OnLobbyCreated;

	/** Search finished (guest). Results are empty on failure. */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Lobby")
	FSnLobbiesFoundDelegate OnLobbiesFound;

	/** Join finished (guest): true = travel to the host has started. */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Lobby")
	FSnLobbyJoinedDelegate OnLobbyJoined;

	/** Session destroyed (host or guest). */
	UPROPERTY(BlueprintAssignable, Category = "Sn|Lobby")
	FSnLobbyDestroyedDelegate OnLobbyDestroyed;

private:
	/** The engine session interface of the default online subsystem. */
	IOnlineSession* GetSessionInterface() const;

	// Engine-delegate callbacks (bound via CreateUObject on the Add*Delegate_Handle
	// path). Plain members, no UFUNCTION: the callback signatures carry OSS types
	// like EOnJoinSessionCompleteResult::Type that UHT cannot parse in a
	// UFUNCTION() declaration, and CreateUObject binding needs no reflection.

	void HandleCreateCompleted(FName SessionName, bool bWasSuccessful);

	void HandleFindCompleted(bool bWasSuccessful);

	void HandleJoinCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	void HandleDestroyCompleted(FName SessionName, bool bWasSuccessful);

	/** True while a LAN search is running (guards double searches). */
	bool bSearchInProgress = false;

	/** Search results backing the last OnLobbiesFound. */
	TSharedPtr<FOnlineSessionSearch> SearchObject;

	FDelegateHandle CreateCompleteHandle;
	FDelegateHandle FindCompleteHandle;
	FDelegateHandle JoinCompleteHandle;
	FDelegateHandle DestroyCompleteHandle;

	bool bIsHosting = false;
	bool bIsInLobby = false;
};
