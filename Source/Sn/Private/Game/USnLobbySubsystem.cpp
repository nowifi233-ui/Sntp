// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/USnLobbySubsystem.h"

#include "Engine/World.h"
#include "Game/USnGameInstance.h"
#include "GameFramework/PlayerController.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h" // SEARCH_LOBBIES
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h" // Online::GetSubsystem(World)

namespace
{
	/** 5.7's FOnlineSessionSettings::Set is a template on the setting struct;
	 *  FOnlineSessionSetting carries the advertised value. */
	void SetSessionSetting(FOnlineSessionSettings& Settings, FName Key, const FString& Value)
	{
		Settings.Settings.Add(Key, FOnlineSessionSetting(Value, EOnlineDataAdvertisementType::ViaOnlineService));
	}
}

void USnLobbySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void USnLobbySubsystem::Deinitialize()
{
	// Game exit: drop any session so the next launch starts clean (a stale
	// advertised session on this machine would confuse LAN searches).
	if (IOnlineSession* Sessions = GetSessionInterface())
	{
		if (Sessions->GetNamedSession(NAME_GameSession))
		{
			Sessions->DestroySession(NAME_GameSession);
		}
	}

	Super::Deinitialize();
}

IOnlineSession* USnLobbySubsystem::GetSessionInterface() const
{
	// Resolve the subsystem through the world context first, exactly like the
	// engine's blueprint proxies (Online::GetSubsystem(World)): under the
	// editor, PIE worlds map to per-world instances ("Null:PIE_N") that own a
	// separate LAN beacon each - one PIE world's search can never kill another
	// world's host beacon. The plain IOnlineSubsystem::Get() (shared default
	// instance) is the no-world / non-editor fallback.
	IOnlineSubsystem* Subsystem = nullptr;
	if (UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		Subsystem = Online::GetSubsystem(World);
	}
	if (!Subsystem)
	{
		Subsystem = IOnlineSubsystem::Get();
	}
	return Subsystem ? Subsystem->GetSessionInterface().Get() : nullptr;
}

void USnLobbySubsystem::CreateLobby(int32 MaxPlayers, const FString& HostName)
{
	if (bIsInLobby)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnLobbySubsystem::CreateLobby: already in a session"));
		OnLobbyCreated.Broadcast(false);
		return;
	}

	IOnlineSession* Sessions = GetSessionInterface();
	if (!Sessions)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnLobbySubsystem::CreateLobby: no online subsystem (enable OnlineSubsystemNull in the project plugins)"));
		OnLobbyCreated.Broadcast(false);
		return;
	}

	// LAN room: advertised over the Null subsystem's LAN beacon, joined by
	// searching the same beacon. HostName (the player ID) rides as a custom
	// setting so the lobby list can show who hosts.
	// The presence/lobby flags mirror the engine's Create Session blueprint
	// node (CreateSessionCallbackProxy) exactly: native nodes find this
	// session reliably on LAN, so the manual path must not diverge from them.
	FOnlineSessionSettings Settings;
	Settings.NumPublicConnections = FMath::Max(MaxPlayers, 2);
	Settings.bShouldAdvertise = true;
	Settings.bAllowJoinInProgress = true;
	Settings.bIsLANMatch = true;
	Settings.bUsesPresence = true;
	Settings.bAllowJoinViaPresence = true;
	Settings.bUseLobbiesIfAvailable = true;
	SetSessionSetting(Settings, TEXT("HostName"), HostName);

	bIsHosting = true;
	bIsInLobby = true;
	CreateCompleteHandle = Sessions->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &USnLobbySubsystem::HandleCreateCompleted));
	if (!Sessions->CreateSession(0, NAME_GameSession, Settings))
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteHandle);
		bIsHosting = false;
		bIsInLobby = false;
		OnLobbyCreated.Broadcast(false);
	}
}

void USnLobbySubsystem::FindLobbies()
{
	if (bSearchInProgress)
	{
		return;
	}

	IOnlineSession* Sessions = GetSessionInterface();
	if (!Sessions)
	{
		OnLobbiesFound.Broadcast(TArray<FBlueprintSessionResult>(), false);
		return;
	}

	// LAN query: 5.7's Null subsystem resolves any FindSessions straight to
	// its beacon broadcast (OnlineSessionInterfaceNull.cpp: FindSessions ->
	// FindLANSession, no query-settings gate). SEARCH_LAN no longer exists in
	// 5.7 - bIsLanQuery is the whole configuration. The lobby query setting
	// mirrors the engine's Find Sessions blueprint node
	// (FindSessionsCallbackProxy), so the manual path behaves identically to
	// the native one.
	SearchObject = MakeShareable(new FOnlineSessionSearch());
	SearchObject->MaxSearchResults = 16;
	SearchObject->bIsLanQuery = true;
	SearchObject->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);

	bSearchInProgress = true;
	FindCompleteHandle = Sessions->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &USnLobbySubsystem::HandleFindCompleted));
	if (!Sessions->FindSessions(0, SearchObject.ToSharedRef()))
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindCompleteHandle);
		bSearchInProgress = false;
		OnLobbiesFound.Broadcast(TArray<FBlueprintSessionResult>(), false);
	}
}

void USnLobbySubsystem::JoinLobby(const FBlueprintSessionResult& LobbyResult)
{
	if (bIsInLobby)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnLobbySubsystem::JoinLobby: already in a session"));
		OnLobbyJoined.Broadcast(false);
		return;
	}

	IOnlineSession* Sessions = GetSessionInterface();
	if (!Sessions)
	{
		OnLobbyJoined.Broadcast(false);
		return;
	}

	JoinCompleteHandle = Sessions->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &USnLobbySubsystem::HandleJoinCompleted));
	if (!Sessions->JoinSession(0, NAME_GameSession, LobbyResult.OnlineResult))
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteHandle);
		OnLobbyJoined.Broadcast(false);
	}
}

void USnLobbySubsystem::DestroyLobby()
{
	IOnlineSession* Sessions = GetSessionInterface();

	// The session interface's own state is the truth here, not our flags: a
	// guest dropped by the host (or a teardown that failed earlier) can leave
	// bIsInLobby stale, and a session left behind blocks CreateLobby and
	// JoinLobby from the menu with no way to clear it. GetNamedSession is the
	// same probe Deinitialize uses, which also makes this idempotent - safe to
	// call on the way into the menu for a machine that has no session at all.
	if (!Sessions || !Sessions->GetNamedSession(NAME_GameSession))
	{
		bIsHosting = false;
		bIsInLobby = false;
		return;
	}

	// Stop claiming to be the host right away: the destroy is asynchronous, and
	// a travel queued behind this call (back to the main menu) must not open a
	// listen server for a room that is already on its way out -
	// USnGameInstance::ServerTravelNow reads this flag to append ?listen.
	// bIsInLobby stays true until the callback: a second DestroyLobby while the
	// teardown is in flight is a no-op, and CreateLobby stays rejected for the
	// session name that is still occupied.
	bIsHosting = false;

	DestroyCompleteHandle = Sessions->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &USnLobbySubsystem::HandleDestroyCompleted));
	if (!Sessions->DestroySession(NAME_GameSession))
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyCompleteHandle);
		bIsInLobby = false;
		OnLobbyDestroyed.Broadcast(false);
	}
}

FString USnLobbySubsystem::GetLobbyDisplayName(const FBlueprintSessionResult& LobbyResult)
{
	FString HostName;
	if (LobbyResult.OnlineResult.Session.SessionSettings.Get(TEXT("HostName"), HostName) && !HostName.IsEmpty())
	{
		return HostName;
	}
	return LobbyResult.OnlineResult.Session.OwningUserName;
}

void USnLobbySubsystem::HandleCreateCompleted(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSession* Sessions = GetSessionInterface())
	{
		Sessions->ClearOnCreateSessionCompleteDelegate_Handle(CreateCompleteHandle);
	}

	if (bWasSuccessful)
	{
		// Start the session so the LAN beacon advertises it. No result
		// handler: Null's start is synchronous, and the create succeeded
		// either way (the Proxy path does the same ordering).
		if (IOnlineSession* Sessions = GetSessionInterface())
		{
			Sessions->StartSession(NAME_GameSession);
		}
	}
	else
	{
		bIsHosting = false;
		bIsInLobby = false;
	}

	OnLobbyCreated.Broadcast(bWasSuccessful);
}

void USnLobbySubsystem::HandleFindCompleted(bool bWasSuccessful)
{
	bSearchInProgress = false;
	if (IOnlineSession* Sessions = GetSessionInterface())
	{
		Sessions->ClearOnFindSessionsCompleteDelegate_Handle(FindCompleteHandle);
	}

	TArray<FBlueprintSessionResult> Results;
	if (bWasSuccessful && SearchObject.IsValid())
	{
		Results.Reserve(SearchObject->SearchResults.Num());
		for (const FOnlineSessionSearchResult& SearchResult : SearchObject->SearchResults)
		{
			FBlueprintSessionResult Result;
			Result.OnlineResult = SearchResult;
			Results.Add(Result);
		}
	}

	OnLobbiesFound.Broadcast(Results, bWasSuccessful);
}

void USnLobbySubsystem::HandleJoinCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (IOnlineSession* Sessions = GetSessionInterface())
	{
		Sessions->ClearOnJoinSessionCompleteDelegate_Handle(JoinCompleteHandle);
	}

	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		OnLobbyJoined.Broadcast(false);
		return;
	}

	bIsInLobby = true;

	// Join only handed us the session info - the actual connection is a
	// client travel to the host address (from the LAN beacon), over the
	// default IP net driver.
	if (IOnlineSession* Sessions = GetSessionInterface())
	{
		FString ConnectString;
		if (Sessions->GetResolvedConnectString(NAME_GameSession, ConnectString, NAME_GamePort))
		{
			if (UWorld* World = GetGameInstance()->GetWorld())
			{
				if (APlayerController* PC = World->GetFirstPlayerController())
				{
					// This machine is in a session from here on, marked BEFORE
					// the travel - the same place the host's StartNewGame marks
					// it. The next world's PlayerController::BeginPlay repairs
					// the viewport (UI-only input state and mouse capture
					// outlive the level, see its comment) only while that flag
					// is set: a guest that skips this keeps the menu's UI-only
					// viewport and cannot move until the host happens to travel.
					if (USnGameInstance* SnGameInstance = World->GetGameInstance<USnGameInstance>())
					{
						SnGameInstance->SetInSession();
					}

					PC->ClientTravel(ConnectString, TRAVEL_Absolute);
					OnLobbyJoined.Broadcast(true);
					return;
				}
			}
		}
	}

	bIsInLobby = false;
	OnLobbyJoined.Broadcast(false);
}

void USnLobbySubsystem::HandleDestroyCompleted(FName SessionName, bool bWasSuccessful)
{
	if (IOnlineSession* Sessions = GetSessionInterface())
	{
		Sessions->ClearOnDestroySessionCompleteDelegate_Handle(DestroyCompleteHandle);
	}

	bIsHosting = false;
	bIsInLobby = false;
	OnLobbyDestroyed.Broadcast(bWasSuccessful);
}
