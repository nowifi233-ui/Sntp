// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/USnGameInstance.h"

#include "Game/Save/USnSaveGame.h"
#include "Game/USnLobbySubsystem.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "MoviePlayer.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"
#include "Game/SnSettings.h"
#include "Game/USnBuildLibrary.h"
#include "TimerManager.h"
#include "UI/SnLoadingScreen.h"

namespace
{
	constexpr int32 SaveUserIndex = 0;

	/** Length cap shared by player IDs and save slot names - both become part of
	 *  a file name, and both are validated by IsValidPlayerID. */
	constexpr int32 MaxIDLength = 16;

	/** How long the travel barrier waits for a client that never confirms. */
	constexpr float TravelBarrierTimeoutSeconds = 3.0f;

	/** Session key for THIS machine's own player when it never bound an ID (see
	 *  GetSessionKey). '%' is not a legal profile-ID character
	 *  (IsValidPlayerID), so a real profile can never collide with it - and a
	 *  machine in this state writes no file, so it never reaches disk. */
	const FString LocalProfileKey = TEXT("%Local%");

	/** Where the remembered player ID lives: the user's own config, not the
	 *  project's - it identifies the person at this machine, like a Minecraft
	 *  profile, and belongs with their other local settings. */
	const TCHAR* const PlayerIDConfigSection = TEXT("Sn.Player");
	const TCHAR* const PlayerIDConfigKey = TEXT("PlayerID");
}

void USnGameInstance::Init()
{
	Super::Init();

	// Covers the boot map load: Init runs while the game instance is being created,
	// and the engine browses to the default map right after it.
	SetupLoadingScreen();

	// ...and this re-arms every later one. The movie player does NOT keep the
	// attributes it was given: a finished play wipes them ("forcing users to always
	// explicitly set the loading screen they want rather than have stale loading
	// screens" - DefaultGameMoviePlayer.cpp), so the next travel's PlayMovie found
	// nothing prepared, broadcast OnPrepareLoadingScreen to nobody, and loaded
	// screenless. That is the whole "loading screen only at launch" symptom: the
	// boot load was the one play the Init call above covered. PlayMovie broadcasts
	// that event and then re-checks what it got, so a handler that re-sets the
	// attributes is all it takes.
	//
	// Disabled player (editor, -NoLoadingScreen): the engine hands back a null
	// player whose SetupLoadingScreen and events are no-ops, so the binding just
	// sits there unused - PIE travel keeps its instant cut.
	if (IGameMoviePlayer* MoviePlayer = GetMoviePlayer())
	{
		MoviePlayer->OnPrepareLoadingScreen().AddUObject(this, &USnGameInstance::SetupLoadingScreen);
	}
}

void USnGameInstance::SetupLoadingScreen()
{
	// The movie player does the showing: it hooks PreLoadMap/PostLoadMapWithWorld
	// itself (FDefaultGameMoviePlayer::Initialize) and plays whatever attributes it
	// holds at that moment - hence one call per map load (Init, then the player's
	// own prepare hook; see Init). Menu -> hub, hub -> run, the return trip and both
	// sides of a LAN travel (each machine loads its own copy) get the screen with
	// nobody starting or stopping it.
	//
	// No editor: the engine disables the movie player under GIsEditor and hands out
	// a null player whose SetupLoadingScreen is a no-op, so nothing here would show
	// anyway - PIE travel keeps its instant cut. Play > Standalone Game is where
	// this lives.
	if (!IsMoviePlayerEnabled())
	{
		return;
	}

	IGameMoviePlayer* MoviePlayer = GetMoviePlayer();
	if (!MoviePlayer)
	{
		return;
	}

	const USnSettings* Settings = USnSettings::Get();

	FLoadingScreenAttributes Attributes;
	// No movie: the widget is the whole screen.
	// The widget is built now, on the game thread. From here on it is ticked and
	// painted on the movie player's Slate loading thread, hence the rule in
	// SSnLoadingScreen: plain values only, no game state.
	Attributes.WidgetLoadingScreen = SnMakeLoadingScreenWidget(
		Settings->GetLoadingScreenTitle(), Settings->GetLoadingScreenText());

	// Hand back the moment the level is ready, but never before the minimum - a
	// fast load would otherwise strobe the screen for two frames. The enforced
	// wait spins on the game thread (WaitForMovieToFinish), which is why the
	// value is configurable and small by default.
	Attributes.bAutoCompleteWhenLoadingCompletes = true;
	Attributes.MinimumLoadingScreenDisplayTime = Settings->GetLoadingScreenMinimumSeconds();

	MoviePlayer->SetupLoadingScreen(Attributes);
}

bool USnGameInstance::IsValidPlayerID(const FString& PlayerID)
{
	if (PlayerID.IsEmpty() || PlayerID.Len() > MaxIDLength)
	{
		return false;
	}
	for (const TCHAR Char : PlayerID)
	{
		if (!FChar::IsAlnum(Char) && Char != TEXT('_'))
		{
			return false;
		}
	}
	return true;
}

bool USnGameInstance::IsValidSaveSlotID(const FString& SaveSlotID)
{
	// One rule, two names: a slot is a file name and so is a player ID's file.
	// The separate entry point exists for the menu's "new save" field, which
	// should not have to ask a question phrased about players.
	return IsValidPlayerID(SaveSlotID);
}

FString USnGameInstance::GetSlotName(const FString& SaveSlot) const
{
	// The slot is validated at every public entry point; the guard here keeps
	// the path construction safe even if a future caller forgets.
	return FString::Printf(TEXT("Save_%s"), *SaveSlot);
}

USnSaveGame* USnGameInstance::LoadSlotFile(const FString& SaveSlot) const
{
	if (!IsValidSaveSlotID(SaveSlot))
	{
		return nullptr;
	}
	return Cast<USnSaveGame>(UGameplayStatics::LoadGameFromSlot(GetSlotName(SaveSlot), SaveUserIndex));
}

bool USnGameInstance::SaveCurrentSlot()
{
	if (CurrentSaveSlot.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveCurrentSlot: no save slot bound yet (the main menu, or a guest)"));
		return false;
	}
	return SaveGameToSlot(CurrentSaveSlot);
}

bool USnGameInstance::SaveGameToSlot(const FString& SaveSlot)
{
	if (!IsValidSaveSlotID(SaveSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveGameToSlot: invalid save slot '%s' (letters/digits/underscore, <= %d chars)"), *SaveSlot, MaxIDLength);
		return false;
	}

	// The table already in the file is the starting point. Outside a session
	// there is nothing live to capture - nothing changes a character at the main
	// menu, so what is on disk is what the players have, and the menu's save
	// button (SaveCurrentSlot) must not wipe a roster a previous session wrote.
	TMap<FString, FSnCharacterSave> Characters;
	if (const USnSaveGame* Existing = LoadSlotFile(SaveSlot))
	{
		if (Existing->SaveVersion == SN_SAVE_VERSION)
		{
			Characters = Existing->Characters;
		}
	}

	if (bInSession)
	{
		// Mid-session the live roster IS the truth: it holds this machine's own
		// player and every guest this room captured (at travels, at leaves). The
		// copy below also carries guests who are no longer here - their entries
		// are what their next join reads back.
		Characters = SessionCharacters;

		// ...minus the placeholder key. It is what the in-memory capture uses for
		// a session that began without an ID (PIE straight into a level, where
		// there is no menu to create one at), and it must never become a key in a
		// file. Dropping it here covers every write, since this is the one place
		// that decides what a save file contains.
		Characters.Remove(LocalProfileKey);

		// ...topped up with this machine's own player, whose entry is refreshed
		// right now: the roster is only rewritten at travel and leave points, and
		// a save can land between them (the pause menu's save button). Index 0 is
		// THIS machine's controller - on a listen server GetFirstPlayerController
		// is merely the first in iteration order (World.cpp) and can be a guest's,
		// while the room in this file belongs to the machine that hosts it.
		if (ASnPlayerState* PS = GetLocalPlayerState())
		{
			PS->FillRunSnapshot(RunSnapshot);

			// Only with an ID to file it under: an unbound machine (PIE straight
			// into a level) captures under the placeholder in memory, and writing
			// that as a key would put a junk profile in the file.
			if (!CurrentPlayerID.IsEmpty())
			{
				PS->FillCharacterSnapshot(Characters.FindOrAdd(CurrentPlayerID));
			}
		}
	}

	// Stamp every entry with its key: the ID is what a character carries when it
	// travels on its own, and the two are written together or not at all.
	for (TPair<FString, FSnCharacterSave>& Entry : Characters)
	{
		Entry.Value.ProfileID = Entry.Key;
	}

	// The room's owner, as a label on the file (the menu reads it to say who a
	// save belongs to). Never cleared by the write: a save made from a machine
	// with no ID bound keeps whatever the file already said.
	if (!CurrentPlayerID.IsEmpty())
	{
		RunSnapshot.HostProfileID = CurrentPlayerID;
	}

	USnSaveGame* Save = NewObject<USnSaveGame>();
	Save->Run = RunSnapshot;
	Save->Characters = MoveTemp(Characters);
	Save->SaveTime = FDateTime::Now();
	Save->SaveVersion = SN_SAVE_VERSION;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(Save, GetSlotName(SaveSlot), SaveUserIndex);
	if (bSaved)
	{
		// This slot is now the one this machine plays in: a write is how the
		// pause menu's save lands on the room the session was started from. The
		// player ID is deliberately NOT touched - the file name says which room,
		// never who you are.
		CurrentSaveSlot = SaveSlot;
		UE_LOG(LogTemp, Log, TEXT("SaveGameToSlot: '%s' saved"), *SaveSlot);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("SaveGameToSlot: failed to save '%s'"), *SaveSlot);
	}
	return bSaved;
}

bool USnGameInstance::BindPlayerID(const FString& PlayerID)
{
	if (!IsValidPlayerID(PlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("BindPlayerID: invalid player ID '%s' (letters/digits/underscore, <= %d chars)"), *PlayerID, MaxIDLength);
		return false;
	}
	SetCurrentPlayerID(PlayerID);
	return true;
}

FString USnGameInstance::GetRememberedPlayerID() const
{
	FString Remembered;
	if (!GConfig)
	{
		return FString();
	}
	GConfig->GetString(PlayerIDConfigSection, PlayerIDConfigKey, Remembered, GGameUserSettingsIni);

	// Never hand back something the rest of the system would reject: a config
	// edited by hand has to fail the same test as an ID typed at the menu, or
	// the menu would pre-fill a field that cannot be confirmed.
	return IsValidPlayerID(Remembered) ? Remembered : FString();
}

void USnGameInstance::SetCurrentPlayerID(const FString& PlayerID)
{
	CurrentPlayerID = PlayerID;
	if (PlayerID.IsEmpty())
	{
		return;
	}

	// A local entry captured before this machine had an ID (a PIE session started
	// straight into a level, or a guest who played a while before binding one) is
	// re-keyed: from here on the local player is looked up by the real ID.
	FSnCharacterSave LocalEntry;
	if (SessionCharacters.RemoveAndCopyValue(LocalProfileKey, LocalEntry))
	{
		SessionCharacters.Add(PlayerID, MoveTemp(LocalEntry));
	}

	// Remembered for next launch. The ID is the player's whole identity - it is
	// what their character is filed under on every machine that has hosted
	// them - so it must not depend on them retyping it correctly every session.
	GConfig->SetString(PlayerIDConfigSection, PlayerIDConfigKey, *PlayerID, GGameUserSettingsIni);
	GConfig->Flush(/*bRead=*/false, GGameUserSettingsIni);
}

bool USnGameInstance::LoadGameFromSlot(const FString& SaveSlot, const FString& PlayerID)
{
	if (!IsValidPlayerID(PlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadGameFromSlot: invalid player ID '%s' (letters/digits/underscore, <= %d chars)"), *PlayerID, MaxIDLength);
		return false;
	}

	USnSaveGame* Loaded = LoadSlotFile(SaveSlot);
	if (!Loaded)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadGameFromSlot: '%s' missing or corrupt"), *SaveSlot);
		return false;
	}

	if (Loaded->SaveVersion != SN_SAVE_VERSION)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadGameFromSlot: '%s' is save version %d, current is %d"),
			*SaveSlot, Loaded->SaveVersion, SN_SAVE_VERSION);
		return false;
	}

	RunSnapshot = Loaded->Run;

	// The whole roster lands as it was written: the file holds a character for
	// every player ID that ever played in this room. The world about to load
	// cannot tell a loaded roster from one a travel captured - the player state's
	// restore is the same single lookup either way - and this is what lets a
	// guest rejoin the room with the character they left with, instead of
	// starting over on a white board.
	SessionCharacters = Loaded->Characters;

	// Arms the run restore for the new world: difficulty for its GameState,
	// and - on a continue entry only - the saved position (bRestoreLocation is
	// set by ContinueGame, cleared by every travel).
	bRunPending = true;

	// Which room this machine is now in, and who it is in it. The two are set
	// together because entering a room needs both, but they stay independent:
	// the same file can be entered as a different player (a different character
	// comes out of the roster), and a player is not tied to one file.
	CurrentSaveSlot = SaveSlot;
	SetCurrentPlayerID(PlayerID);
	return true;
}

void USnGameInstance::DeleteSaveSlot(const FString& SaveSlot)
{
	if (!IsValidSaveSlotID(SaveSlot))
	{
		return;
	}
	UGameplayStatics::DeleteGameInSlot(GetSlotName(SaveSlot), SaveUserIndex);
}

bool USnGameInstance::HasSaveInSlot(const FString& SaveSlot) const
{
	if (!IsValidSaveSlotID(SaveSlot))
	{
		return false;
	}
	return UGameplayStatics::DoesSaveGameExist(GetSlotName(SaveSlot), SaveUserIndex);
}

TArray<FSnSaveSlotInfo> USnGameInstance::GetSaveSlotInfos()
{
	TArray<FSnSaveSlotInfo> Infos;

	// Scan the save directory for "Save_*.sav" files - each file is one room, so
	// the list is whatever exists, not a fixed slot count.
	TArray<FString> FoundFiles;
	const FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager::Get().FindFiles(FoundFiles, *SaveDir, TEXT("*.sav"));

	for (const FString& FileName : FoundFiles)
	{
		if (!FileName.StartsWith(TEXT("Save_")))
		{
			continue;
		}
		FString SaveSlot = FileName;
		SaveSlot.RemoveFromStart(TEXT("Save_"));
		SaveSlot.RemoveFromEnd(TEXT(".sav"));

		FSnSaveSlotInfo Info;
		Info.SaveSlotID = SaveSlot;
		if (USnSaveGame* Save = LoadSlotFile(SaveSlot))
		{
			Info.bOccupied = true;
			Info.SaveTime = Save->SaveTime;
			Info.LevelName = Save->Run.LevelName;
			Info.HostProfileID = Save->Run.HostProfileID;
			Info.CharacterCount = Save->Characters.Num();

			// The summary line describes the room's OWNER: the file holds a whole
			// party's characters, and the level and wallet on that line are the
			// host's - the one character that is certainly this room's author. No
			// such entry (a file written before the field existed) leaves the
			// defaults, which is what an untouched slot reads as.
			if (const FSnCharacterSave* Host = Save->Characters.Find(Save->Run.HostProfileID))
			{
				Info.PlayerLevel = Host->Level;
				Info.Gold = Host->Gold;
			}
		}
		Infos.Add(Info);
	}

	// Newest first - the menu shows recent sessions on top.
	Infos.Sort([](const FSnSaveSlotInfo& A, const FSnSaveSlotInfo& B)
		{ return A.SaveTime > B.SaveTime; });
	return Infos;
}

void USnGameInstance::StartNewGame(const FString& SaveSlot, const FString& PlayerID)
{
	if (!IsValidSaveSlotID(SaveSlot))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartNewGame: invalid save slot '%s' (letters/digits/underscore, <= %d chars)"), *SaveSlot, MaxIDLength);
		return;
	}
	if (!IsValidPlayerID(PlayerID))
	{
		UE_LOG(LogTemp, Warning, TEXT("StartNewGame: invalid player ID '%s' (letters/digits/underscore, <= %d chars)"), *PlayerID, MaxIDLength);
		return;
	}

	ResetSnapshot();
	// After the reset: the slot names the room being created, the ID names who
	// plays in it, and binding re-keys the local player's entry (the roster was
	// just emptied - a fresh game starts everyone from scratch).
	CurrentSaveSlot = SaveSlot;
	SetCurrentPlayerID(PlayerID);
	bInSession = true;

	const FString StartLevelName = USnSettings::Get()->GetStartLevelName();
	if (StartLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("StartNewGame: USnSettings::StartLevel is not set"));
		return;
	}
	ServerTravelNow(StartLevelName);
}

void USnGameInstance::ContinueGame(const FString& SaveSlot, const FString& PlayerID)
{
	if (!LoadGameFromSlot(SaveSlot, PlayerID))
	{
		return;
	}

	bInSession = true;

	// Continue-game entries restore the saved position (the player state's
	// TryRestoreFromSave reads this); level-to-level travel keeps it cleared.
	bRestoreLocation = true;

	if (RunSnapshot.LevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ContinueGame: '%s' has no level, falling back to the start level"), *SaveSlot);
		ServerTravelNow(USnSettings::Get()->GetStartLevelName());
		return;
	}
	ServerTravelNow(RunSnapshot.LevelName);
}

void USnGameInstance::PrepareServerTravel(const FString& LevelName)
{
	// The next world's PlayerStates rebuild from the in-memory capture -
	// independent of disk saving: works in PIE with no bound ID (and no
	// bInSession), and survives disk failures. Writing is best-effort on top.
	// Server-authoritative: the room's own fields come from THIS machine's
	// player state, and every player's character is captured - on a listen
	// server the guests' states are rebuilt here as well, and without their own
	// entry they would come back as class defaults (empty bag, level 1).
	if (ASnPlayerState* LocalPS = GetLocalPlayerState())
	{
		LocalPS->FillRunSnapshot(RunSnapshot);
	}
	CaptureSessionCharacters();

	// The slot says whether this machine is a room's owner: it is set by
	// StartNewGame/ContinueGame and only ever written back to. A guest has none
	// (it bound an ID and joined), which is what keeps "a guest writes no file"
	// true without a separate rule.
	if (!CurrentSaveSlot.IsEmpty())
	{
		if (!SaveGameToSlot(CurrentSaveSlot))
		{
			UE_LOG(LogTemp, Warning, TEXT("PrepareServerTravel: autosave failed (in-memory capture still passed on)"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareServerTravel: no save slot bound, skipping disk save (in-memory capture passed on)"));
	}

	// Arm the restore: without this a level travel resets attributes,
	// inventory and level back to defaults.
	bRunPending = true;

	// Map-to-map travel spawns at the PlayerStart - the saved position is
	// only for continue-game entries (see ShouldRestoreLocation).
	bRestoreLocation = false;

	if (LevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("PrepareServerTravel: empty level name"));
	}
}

void USnGameInstance::ServerTravelNow(const FString& LevelName)
{
	UWorld* World = GetWorld();
	if (!World || LevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ServerTravelNow: empty level name or no world"));
		return;
	}

	// Returning to the main menu ends the session: the menu level must never
	// run the in-session teardown or take part in save guarding (bInSession
	// gates both in PlayerController::BeginPlay and SaveGameToSlot). Asked
	// through the settings' own test - callers name the menu level in whatever
	// notation their blueprint had at hand (see USnSettings::IsMainMenuLevel).
	if (USnSettings::IsMainMenuLevel(LevelName))
	{
		bInSession = false;
	}

	FString Target = LevelName;
	// Hosting a lobby: the server-side travel must keep the ListenServer
	// alive, or every client drops on arrival. Standalone/single-player
	// travels just load the level.
	if (USnLobbySubsystem* Lobby = World->GetGameInstance()->GetSubsystem<USnLobbySubsystem>())
	{
		if (Lobby->IsHosting())
		{
			Target += TEXT("?listen");
		}
	}

	// This machine is about to lose its world: close its windows and re-sync its
	// input mode in THIS frame. Every server-side travel funnels through here,
	// which is why the teardown sits here and not at the callers -
	// StartNewGame/ContinueGame travel straight out of the main menu window and
	// used to skip it, leaving the game viewport client in UI-only across the
	// load, while the next world's controller starts with an empty window
	// manager and cannot tell a window was ever open. The player arrived in the
	// run map unable to move. GetPlayerController(World, 0) is THIS machine's
	// controller (index 0 counts local players first), so a listen server never
	// reaches into a remote client's UI.
	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(UGameplayStatics::GetPlayerController(World, 0)))
	{
		SnPC->CloseAllWindowsAndApplyInputMode();
	}

	// MUST be UWorld::ServerTravel, never UGameplayStatics::OpenLevel. OpenLevel
	// bottoms out in GEngine->SetClientTravel, which only sets THIS machine's
	// travel URL - it never reaches AGameModeBase::ProcessClientTravel, the one
	// place that sends the ClientTravel RPC to each remote client. Through
	// OpenLevel the server alone moves and every client is stranded on a
	// connection that is about to die (the engine then drops them to
	// GameDefaultMap). ProcessClientTravel also strips "?listen" from the URL
	// it sends, which is exactly right: the clients must not think they are the
	// server.
	if (!World->ServerTravel(Target, /*bAbsolute=*/true))
	{
		// AGameModeBase::CanServerTravel rejected it (illegal characters) or the
		// world is already mid-travel. Without this the team would sit in a
		// level nobody is leaving with no clue why.
		UE_LOG(LogTemp, Warning, TEXT("ServerTravelNow: ServerTravel refused '%s'"), *Target);
	}
}

void USnGameInstance::BeginTravelBarrier(const FString& LevelName, int32 RemoteClientCount)
{
	PendingTravelLevel = LevelName;
	TravelConfirmedClients.Reset();
	TravelConfirmationsRemaining = RemoteClientCount;

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// The timeout lives on the current world's timer: if the world goes away
	// without the barrier completing, the stale timer goes with it instead of
	// firing a travel in the next level.
	World->GetTimerManager().SetTimer(TravelTimeoutHandle, this, &USnGameInstance::CompleteServerTravel, TravelBarrierTimeoutSeconds, /*bLoop=*/false);

	if (TravelConfirmationsRemaining <= 0)
	{
		// Standalone (single-player) or no remote clients: travel immediately.
		CompleteServerTravel();
	}
}

void USnGameInstance::ConfirmTravel(APlayerController* Client)
{
	if (PendingTravelLevel.IsEmpty() || !Client || TravelConfirmedClients.Contains(Client))
	{
		return;
	}

	TravelConfirmedClients.Add(Client);
	if (TravelConfirmedClients.Num() >= TravelConfirmationsRemaining)
	{
		CompleteServerTravel();
	}
}

void USnGameInstance::CompleteServerTravel()
{
	// Read-and-clear before travelling: ServerTravelNow loads the next world,
	// and the barrier must not be left armed for it.
	const FString LevelName = PendingTravelLevel;
	PendingTravelLevel.Reset();
	TravelConfirmedClients.Reset();
	TravelConfirmationsRemaining = 0;

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TravelTimeoutHandle);
	}

	if (LevelName.IsEmpty())
	{
		return;
	}

	ServerTravelNow(LevelName);
}

void USnGameInstance::AddMilestone(FName MilestoneID)
{
	if (MilestoneID.IsNone() || RunSnapshot.Milestones.Contains(MilestoneID))
	{
		return;
	}
	RunSnapshot.Milestones.Add(MilestoneID);
}

AActor* USnGameInstance::AddBuiltDevice(FName DeviceID, const FVector& Location, const FRotator& Rotation)
{
	UWorld* World = GetWorld();
	const FSnBuildDeviceRow* Row = USnBuildLibrary::FindBuildDeviceRow(DeviceID);
	if (!World || !Row || !Row->DeviceClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("AddBuiltDevice: unknown device %s"), *DeviceID.ToString());
		return nullptr;
	}

	FSnBuiltDevice Record;
	Record.DeviceID = DeviceID;
	Record.Location = Location;
	Record.Rotation = Rotation;
	// Which level owns this record: the same normalization the snapshot's
	// LevelName uses, so the two agree on what a level is called.
	Record.LevelName = USnBuildLibrary::GetBuildLevelName(World);
	RunSnapshot.BuiltDevices.Add(Record);

	return World->SpawnActor<AActor>(Row->DeviceClass, Location, Rotation);
}

TArray<AActor*> USnGameInstance::SpawnBuiltDevices()
{
	TArray<AActor*> Spawned;
	UWorld* World = GetWorld();
	if (!World)
	{
		return Spawned;
	}

	// Filter by owner level: devices can be placed in any zone, so a record that
	// belongs to another level must not be rebuilt here (and records written
	// before the field existed carry no level and keep replaying everywhere).
	const FString ThisLevel = USnBuildLibrary::GetBuildLevelName(World);
	for (const FSnBuiltDevice& Record : RunSnapshot.BuiltDevices)
	{
		if (!Record.LevelName.IsEmpty() && Record.LevelName != ThisLevel)
		{
			continue;
		}

		const FSnBuildDeviceRow* Row = USnBuildLibrary::FindBuildDeviceRow(Record.DeviceID);
		if (Row && Row->DeviceClass)
		{
			if (AActor* Device = World->SpawnActor<AActor>(Row->DeviceClass, Record.Location, Record.Rotation))
			{
				Spawned.Add(Device);
			}
		}
	}
	return Spawned;
}

void USnGameInstance::ResetSnapshot()
{
	RunSnapshot = FSnRunSave();
	SessionCharacters.Reset();
	bRunPending = false;
	bRestoreLocation = false;
}

ASnPlayerState* USnGameInstance::GetLocalPlayerState() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Index 0 = the first LOCAL player's controller, deliberately not
	// World->GetFirstPlayerController(): that one is simply whatever the
	// controller iterator yields first (World.cpp:6525), and the engine warns
	// the order is not stable across map transfers (GameplayStatics.cpp:317) -
	// what this class captures for the room must be THIS machine's player, not
	// whoever ends up first in the list after a travel.
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	return PC ? PC->GetPlayerState<ASnPlayerState>() : nullptr;
}

bool USnGameInstance::IsLocalPlayerState(const ASnPlayerState* PS) const
{
	return PS != nullptr && PS == GetLocalPlayerState();
}

FString USnGameInstance::GetSessionKey(const ASnPlayerState* PS) const
{
	if (!PS)
	{
		return FString();
	}

	if (IsLocalPlayerState(PS))
	{
		// This machine's own player is filed under the ID it bound before
		// entering the game. The placeholder only covers a session that began
		// without one - PIE straight into a level, where there is no menu to
		// create an ID at - so that the in-memory capture still carries the
		// character across a travel. Such a machine writes no file, so the
		// placeholder never becomes a key on disk.
		return CurrentPlayerID.IsEmpty() ? LocalProfileKey : CurrentPlayerID;
	}

	// A remote player is identified by the profile ID they reported (see
	// ASnPlayerController::ServerSetPlayerName). Without one they are
	// indistinguishable from the next anonymous guest, so nothing is kept.
	return PS->GetProfileID();
}

void USnGameInstance::CaptureSessionCharacters()
{
	UWorld* World = GetWorld();
	AGameStateBase* GameState = World ? World->GetGameState() : nullptr;
	if (!GameState)
	{
		return;
	}

	// Refreshed in place, never cleared: the roster is the room's record of
	// everyone who has played here, and it also holds players who are no longer
	// in this world (their characters are what their next join reads back).
	//
	// PlayerArray covers the local player and every guest: on a listen server
	// all of their PlayerStates live here, and the level switch is about to
	// destroy every one of them.
	for (const TObjectPtr<APlayerState>& Entry : GameState->PlayerArray)
	{
		ASnPlayerState* PS = Cast<ASnPlayerState>(Entry.Get());
		if (!PS)
		{
			continue;
		}

		const FString Key = GetSessionKey(PS);
		if (Key.IsEmpty())
		{
			// A guest who never bound a profile (or bound one this room already
			// had): they can keep playing, but their character has nowhere to go
			// (see BindPlayerID).
			UE_LOG(LogTemp, Log, TEXT("CaptureSessionCharacters: '%s' reported no profile ID, character not carried over"), *PS->GetPlayerName());
			continue;
		}

		PS->FillCharacterSnapshot(SessionCharacters.FindOrAdd(Key));
	}
}

bool USnGameInstance::GetSessionCharacter(const ASnPlayerState* PS, FSnCharacterSave& OutCharacter) const
{
	const FString Key = GetSessionKey(PS);
	if (Key.IsEmpty())
	{
		return false;
	}

	// Read, never consumed: the entry belongs to the room and every level switch
	// asks for it again. What stops a second application from doubling a
	// player's equipment is the player state's own bCharacterRestored flag -
	// once per state, which is exactly once per world (a travel builds new ones)
	// and never twice for a respawn (which keeps the same one).
	if (const FSnCharacterSave* Found = SessionCharacters.Find(Key))
	{
		OutCharacter = *Found;
		return true;
	}
	return false;
}

void USnGameInstance::SaveDepartedCharacter(ASnPlayerState* PS)
{
	if (!PS || !bInSession)
	{
		return;
	}

	// A level switch destroys every controller too, and would land here once per
	// player for a roster that PrepareServerTravel already captured and wrote.
	// The engine sets this before it does any of that teardown (UEngine::LoadMap
	// calls BeginTearingDown first), so it is the one signal that separates "a
	// player is leaving" from "the world is going away under all of us".
	const UWorld* World = GetWorld();
	if (World && World->bIsTearingDown)
	{
		return;
	}

	const FString Key = GetSessionKey(PS);
	if (Key.IsEmpty())
	{
		return;
	}

	// Everything this player did since the last travel lives only in this state,
	// which is about to be destroyed - take it now, while there is still a state
	// to take it from.
	PS->FillCharacterSnapshot(SessionCharacters.FindOrAdd(Key));

	// Only the room's own file is written, and only by the machine that owns it:
	// a guest never binds a save slot (it joined someone else's room), so this
	// is a no-op there by construction.
	if (!CurrentSaveSlot.IsEmpty())
	{
		SaveGameToSlot(CurrentSaveSlot);
	}
}

bool USnGameInstance::TakePendingRun(FSnRunSave& OutRun)
{
	if (!bRunPending)
	{
		return false;
	}

	bRunPending = false;
	OutRun = RunSnapshot;
	return true;
}
