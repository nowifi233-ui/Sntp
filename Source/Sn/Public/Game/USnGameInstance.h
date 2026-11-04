// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Engine/TimerHandle.h"
#include "Game/Save/SnSaveTypes.h"
#include "USnGameInstance.generated.h"

class ASnPlayerState;
class USnLobbySubsystem;
class USnSaveGame;

/**
 * Session layer of the save system: holds the live state between levels and
 * talks to disk (USnSaveGame via SaveGameToSlot). The single persistence path
 * - travel captures here, quits save here, the main menu lists/deletes/
 * continues through here.
 *
 * A save file is a ROOM, named after a save SLOT ("Save_<Slot>.sav"): the run
 * (progress, base, position, difficulty) plus the characters of everyone who
 * played in it, keyed by player ID. Slot and ID are independent by design -
 * the slot answers "which room", the ID answers "who am I in it" - so the same
 * file can be entered as different people, and a player is not tied to one
 * file. The host keeps the guests' characters: a guest's machine never has a
 * save slot bound and writes no file at all, which is what lets a guest rejoin
 * that room exactly as they left it.
 *
 * Two layers live here:
 *  - RunSnapshot: the room, live for as long as a session runs - milestones
 *    and built devices are written straight into it as they happen, and the
 *    travel capture tops it up with the world-derived fields (level name,
 *    difficulty, the local player's position).
 *  - SessionCharacters: the room's roster - every player's character keyed by
 *    player ID, captured server-side at every travel, refreshed when a player
 *    leaves, and seeded from the host's file on a load. Each entry is handed
 *    back to that player's PlayerState in the next world. A guest's
 *    PlayerState is rebuilt on THIS machine too, which is why the restore
 *    cannot be one session flag.
 *
 * The run only stores differences against the level templates: milestones
 * drive hub scene changes (the hub renders itself from them on load),
 * built-device records are replayed on BeginPlay by the level that owns them
 * (FSnBuiltDevice::LevelName).
 */
UCLASS()
class SN_API USnGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** Arms the travel loading screen: once here for the boot map load (Init runs
	 *  before the engine browses to the default map), and then again before every
	 *  later one - the movie player wipes its attributes after each play and asks
	 *  for new ones through SetupLoadingScreen, which Init binds as the handler. */
	virtual void Init() override;

	// --- Slot management (main menu) ---

	/** Validates a player ID (1-16 chars of letters/digits/underscore - it
	 *  becomes file content and a replication key, so nothing else is allowed). */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	static bool IsValidPlayerID(const FString& PlayerID);

	/** Validates a save slot name - the menu's "new save" field asks it. Same
	 *  rule as a player ID (a slot is a file name too), exposed under its own
	 *  name so the field validates against the thing it is actually typing. */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	static bool IsValidSaveSlotID(const FString& SaveSlotID);

	/** Saves the live snapshot to the slot this machine is playing in (no-op
	 *  when no slot is bound yet - a guest, or a machine at the main menu). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	bool SaveCurrentSlot();

	/** Captures the run and this machine's own character, stamps time/version,
	 *  and writes "Save_<SaveSlot>.sav". Also makes that slot the current one.
	 *  The bound player ID is NOT touched: the file's name says which room,
	 *  never who you are. Returns false on an invalid slot or a failed write. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	bool SaveGameToSlot(const FString& SaveSlot);

	/** Binds PlayerID as THIS machine's profile without saving or travelling -
	 *  the LAN guest's entry point, called before joining. A guest never runs
	 *  StartNewGame/ContinueGame and never binds a save slot, but still needs
	 *  an ID: it is what the host keys the guest's character by (reported
	 *  through ServerSetPlayerName). The ID is remembered for next launch (see
	 *  GetRememberedPlayerID). Returns false on an invalid ID - nothing changes
	 *  then, and a guest who plays on without one is simply not identifiable:
	 *  the host has no key to file their character under, so it does not
	 *  survive the room. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	bool BindPlayerID(const FString& PlayerID);

	/** The ID this machine used last time, for the menu to pre-fill: an ID is
	 *  the player's whole identity (it is what their character is filed under,
	 *  on every machine that has hosted them), so making it something people
	 *  retype from memory is how a typo costs someone their character. Empty
	 *  until an ID has ever been bound, and empty if the stored one is not a
	 *  valid ID (a hand-edited config must fail the same test as typed input). */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	FString GetRememberedPlayerID() const;

	/** Loads a room's file: the run into RunSnapshot, the whole character
	 *  table into SessionCharacters, where the next player states' restores
	 *  pick their own entries up - the same path a travel uses. No travel.
	 *  Both names arrive together because entering a room needs both: SaveSlot
	 *  picks the file, PlayerID picks which character inside it is yours.
	 *  Returns false when either is invalid or the file is missing/unsupported. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	bool LoadGameFromSlot(const FString& SaveSlot, const FString& PlayerID);

	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	void DeleteSaveSlot(const FString& SaveSlot);

	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	bool HasSaveInSlot(const FString& SaveSlot) const;

	/** Summary of every "Save_*" file in the save directory (newest first)
	 *  for the management list. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	TArray<FSnSaveSlotInfo> GetSaveSlotInfos();

	// --- Session flow ---

	/** Starts a NEW room named SaveSlot, played as PlayerID: clears the run and
	 *  every captured character, binds both names, and travels to the start
	 *  level (USnSettings). Hosting a lobby adds ?listen automatically (the menu
	 *  calls this after OnLobbyCreated). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	void StartNewGame(const FString& SaveSlot, const FString& PlayerID);

	/** Loads SaveSlot's room and travels to its level, as PlayerID. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	void ContinueGame(const FString& SaveSlot, const FString& PlayerID);

	// --- Multiplayer travel (server-side) ---

	/** Server-only, called by the travel RPC chain before the actual travel:
	 *  captures the room's world-derived fields, captures EVERY player's
	 *  character (the next world's PlayerStates are rebuilt from scratch - a
	 *  guest's too, on this same machine), writes this machine's file, and
	 *  arms the restore for the next world (map-to-map travel never restores
	 *  the position - PlayerStart is used). */
	void PrepareServerTravel(const FString& LevelName);

	/** Server-only: opens LevelName. While hosting a lobby the travel keeps
	 *  the ListenServer alive ("?listen") and tells every remote client to
	 *  follow. It must go through UWorld::ServerTravel - that is the only path
	 *  reaching AGameModeBase::ProcessClientTravel, which is what actually
	 *  sends the ClientTravel RPCs to the clients. UGameplayStatics::OpenLevel
	 *  only sets THIS machine's travel URL: the server moves, every client is
	 *  left behind on a connection that is about to die, and the engine drops
	 *  them to GameDefaultMap. */
	void ServerTravelNow(const FString& LevelName);

	/** Server-only: arms the travel barrier - opens LevelName once
	 *  RemoteClientCount clients have confirmed (see ConfirmTravel), or after a
	 *  timeout. Travels immediately when there are no remote clients
	 *  (standalone), so single-player keeps its instant travel.
	 *
	 *  The barrier state lives on the GameInstance, not on a player controller:
	 *  ServerTravelReady runs on the CONFIRMING client's controller instance,
	 *  which never saw the request, so per-controller state makes the handshake
	 *  impossible to complete - every LAN travel would just wait out the
	 *  timeout. The GameInstance is also the object that performs the travel
	 *  and outlives the level switch. */
	void BeginTravelBarrier(const FString& LevelName, int32 RemoteClientCount);

	/** Server-only: Client finished its local teardown and is ready for the
	 *  level switch. Deduplicated - an unknown or repeated confirmation is
	 *  ignored (the host's own controller confirms through the same path when
	 *  it was not the requester). */
	void ConfirmTravel(APlayerController* Client);

	// --- Run access (BP reads these; written state lands on disk at the next
	// save). The character half deliberately has no global getter: it lives in
	// the player states and in the per-player capture, never in one member. ---

	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	void SetRunFloor(int32 NewFloor) { RunSnapshot.RunFloor = NewFloor; }

	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	int32 GetRunFloor() const { return RunSnapshot.RunFloor; }

	/** The bound player ID (empty before a game starts/loads). */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	FString GetPlayerID() const { return CurrentPlayerID; }

	/** The save slot this machine is playing in (empty at the main menu, and on
	 *  a guest - it never binds one, which is exactly why it writes nothing). */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	FString GetSaveSlotID() const { return CurrentSaveSlot; }

	// --- Hub progression (milestones) ---

	/** Records a milestone (deduplicated) - e.g. "Floor2Clear". The hub level
	 *  renders its unlocked areas from these on load. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	void AddMilestone(FName MilestoneID);

	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	bool HasMilestone(FName MilestoneID) const { return RunSnapshot.Milestones.Contains(MilestoneID); }

	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	TArray<FName> GetMilestones() const { return RunSnapshot.Milestones; }

	// --- Built devices (per level) ---

	/** Records the device (tagged with THIS level, so only this level replays
	 *  it) and spawns it immediately. Returns the spawned actor (null when the
	 *  device row or its class is missing). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	AActor* AddBuiltDevice(FName DeviceID, const FVector& Location, const FRotator& Rotation);

	/** Replays the devices recorded for the CURRENT level - every level calls
	 *  this on its own BeginPlay (building is not zone-restricted, so a record
	 *  has to say which level owns it). Records from other levels are skipped;
	 *  the records are the source of truth, spawned actors are per-load. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Save")
	TArray<AActor*> SpawnBuiltDevices();

	/** Every record in the run, across all levels - SpawnBuiltDevices is the
	 *  one that filters by level. */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	TArray<FSnBuiltDevice> GetBuiltDevices() const { return RunSnapshot.BuiltDevices; }

	// --- Per-player capture (C++, server-side) ---

	/** Finds the character the room's roster holds for PS. Non-consuming: the
	 *  entry belongs to the room and stays for as long as the room does (every
	 *  level switch asks for it again), so the once-only guard is the player
	 *  state's own flag - see ASnPlayerState::TryRestoreFromSave. False when
	 *  the roster has no entry under this player's key: a guest who just joined
	 *  has no history in this room and keeps the defaults. */
	bool GetSessionCharacter(const ASnPlayerState* PS, FSnCharacterSave& OutCharacter) const;

	/** Server-only, called when a player leaves (AGameModeBase::Logout - the
	 *  engine runs it from AController::Destroyed, the last moment that
	 *  player's state still exists). Captures them into the roster and writes
	 *  the file: everything they did since the last travel would otherwise be
	 *  destroyed with the state. Ignored while the world is tearing down - a
	 *  level switch destroys every controller this way, and that path already
	 *  captured and wrote the whole roster (PrepareServerTravel). */
	void SaveDepartedCharacter(ASnPlayerState* PS);

	/** Handed back once per world entry, and only ever asked for by the local
	 *  player's state: the run-level half of the restore (difficulty for the
	 *  new GameState, the continuing player's position). Keeping it to the
	 *  local player means a guest's state - which initializes on this same
	 *  server - cannot consume it first. */
	bool TakePendingRun(FSnRunSave& OutRun);

	/** True when PS is THIS machine's own player state. A state's owner answers
	 *  "which controller owns it", not "is it mine": on a listen server the
	 *  host's and every guest's state all have an owner (set at spawn by
	 *  AController::InitPlayerState's SpawnInfo, and on clients by the
	 *  replication callback APlayerState::ClientInitialize). This machine's
	 *  player is the project's usual stand-in - GetPlayerController with index
	 *  0, which is the first LOCAL player. Deliberately not
	 *  World::GetFirstPlayerController: that is raw iteration order
	 *  (World.cpp:6525), and the engine itself warns that order is not stable
	 *  across map transfers (GameplayStatics.cpp:317). */
	bool IsLocalPlayerState(const ASnPlayerState* PS) const;

	// --- Internal (C++) ---

	/** True on a continue-game entry: the player's saved position is restored
	 *  instead of the PlayerStart. Cleared on level-to-level travel, which
	 *  always spawns at the PlayerStart. */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	bool ShouldRestoreLocation() const { return bRestoreLocation; }

	/** True while a game is running (started or continued); the main menu is
	 *  not a session. Gameplay code uses this to decide whether menu-only
	 *  input cleanups (e.g. force GameOnly after a level travel) apply. */
	bool IsInSession() const { return bInSession; }

	/** Marks THIS machine as inside a session. The starting/continuing machine
	 *  sets it in StartNewGame / ContinueGame; a JOINING guest never runs those,
	 *  so the LAN join (USnLobbySubsystem::HandleJoinCompleted, right before its
	 *  client travel) sets it here. Without it the new world's
	 *  ASnPlayerController::BeginPlay skips the in-session input repair and the
	 *  guest keeps the main menu's UI-only viewport state - unable to move until
	 *  the host happens to travel, whose ClientPrepareTravel repairs it. */
	void SetInSession() { bInSession = true; }

	/** Marks THIS machine as out of the session. The server-side path does this
	 *  in ServerTravelNow, but a guest returning to the menu travels itself -
	 *  SnPlayerController::LeaveSessionLocally calls this before it goes, or the
	 *  menu's PlayerController::BeginPlay would run the in-session input
	 *  teardown and leave the menu on GameOnly (unclickable). */
	void ClearInSession() { bInSession = false; }

private:
	/** Hands the loading screen widget to the engine movie player: called from
	 *  Init for the boot load, and by the movie player itself before every later
	 *  one (see Init). */
	void SetupLoadingScreen();

	/** "Save_<SaveSlot>.sav" (the slot is validated first - callers must check
	 *  IsValidSaveSlotID). */
	FString GetSlotName(const FString& SaveSlot) const;

	/** Loads a save file without any side effects (no slot or ID binding,
	 *  nothing captured) - the slot list scans every file. */
	USnSaveGame* LoadSlotFile(const FString& SaveSlot) const;

	/** THIS machine's own player state - null in the main menu or before the
	 *  pawn exists. The stand-in for "which state is mine" on the authority
	 *  (see IsLocalPlayerState). */
	ASnPlayerState* GetLocalPlayerState() const;

	/** Binds PlayerID as this machine's profile AND remembers it for next
	 *  launch. The one way CurrentPlayerID is ever set, so the roster key, the
	 *  identity reported to the server and the pre-filled menu field can never
	 *  disagree. Deliberately does not touch the save slot: which room this
	 *  machine is in and who it is in it are separate facts. */
	void SetCurrentPlayerID(const FString& PlayerID);

	/** The roster key for PS. The local player is keyed by the ID this machine
	 *  bound before entering the game; the placeholder covers a session that
	 *  started without one (PIE straight into a level, where there is no menu
	 *  to create an ID at) so the in-memory capture still carries the character
	 *  across a travel - such a machine writes no file, so the placeholder
	 *  never reaches disk. A remote player is keyed by the profile ID they
	 *  reported; an empty key means "not identifiable" (a guest who bound no
	 *  profile, or one whose ID was rejected as a duplicate) and nothing is
	 *  kept for them. */
	FString GetSessionKey(const ASnPlayerState* PS) const;

	/** Refreshes every player's entry in SessionCharacters (server-only, called
	 *  before a travel: the PlayerStates that read the entries back are
	 *  recreated by the level switch, on this machine too). Refreshes in place
	 *  rather than clearing - the roster also holds players who are no longer
	 *  in this world, whose characters their next join will look up. */
	void CaptureSessionCharacters();

	/** Back to fresh-game defaults: the run, every captured character and both
	 *  restore flags. */
	void ResetSnapshot();

	/** Opens PendingTravelLevel: the barrier's normal completion, and the
	 *  timeout catch for a client that never confirms (a connection dropped
	 *  mid-barrier must not freeze the team's travel). */
	void CompleteServerTravel();

	/** Barrier state: the level awaiting confirmation, and the remote clients
	 *  that still have to confirm it. Empty PendingTravelLevel means no travel
	 *  is pending. */
	FString PendingTravelLevel;
	TArray<TObjectPtr<APlayerController>> TravelConfirmedClients;
	int32 TravelConfirmationsRemaining = 0;
	FTimerHandle TravelTimeoutHandle;

	/** The room (see the class comment) - live across levels. */
	FSnRunSave RunSnapshot;

	/** The room's roster: every player's character, keyed by player ID (see
	 *  GetSessionKey). This is the room's record, not a queue - entries are
	 *  refreshed at travels and on leaves, seeded from a loaded file, and read
	 *  back (never consumed) by each player state in each new world. */
	TMap<FString, FSnCharacterSave> SessionCharacters;

	/** The ID this machine's own player is filed under - the roster key here,
	 *  and what the server files this player by when this machine is a guest.
	 *  Empty until a game starts/loads or a LAN guest binds one (the only way it
	 *  is ever set: SetCurrentPlayerID). */
	FString CurrentPlayerID;

	/** The ROOM this machine is playing in - "Save_<this>.sav" is the file it
	 *  writes. Set by StartNewGame / ContinueGame / a successful write, and
	 *  empty on a guest (which is what makes "a guest writes no file" structural
	 *  rather than a rule to remember). Lives on the GameInstance, so it
	 *  survives every level switch of the session. */
	FString CurrentSaveSlot;

	/** Arms the next world's run restore - set by the travel capture and by a
	 *  loaded save, cleared by TakePendingRun. Without the consume-once a
	 *  respawn would re-apply the restored position. */
	bool bRunPending = false;

	/** Set by ContinueGame, cleared by PrepareServerTravel/StartNewGame - see
	 *  ShouldRestoreLocation. */
	bool bRestoreLocation = false;

	/** True once a game started/loaded - the main menu must never capture the
	 *  menu's default player state over a loaded snapshot. */
	bool bInSession = false;
};
