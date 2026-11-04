// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SnPlayerController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Blueprint/UserWidget.h"
#include "Characters/SnCharacter.h"
#include "Dialogue/FSnDialogueRow.h"
#include "Dialogue/USnDialogueController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/DataTable.h"
#include "Engine/World.h" // GetAuthGameMode<T> (unstuck needs the spawn point)
#include "Game/SnCheatManager.h"
#include "Game/SnGameState.h"
#include "Game/SnHUD.h"
#include "Game/USnBuildLibrary.h"
#include "Game/USnGameInstance.h"
#include "Game/USnLobbySubsystem.h"
#include "Interaction/UInteractableComponent.h"
#include "Interaction/UPlayerInteractionComponent.h"
#include "Interaction/USnInteractionController.h"
#include "Loot/ASnItemPickup.h"
#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnItemLibrary.h"
#include "Inventory/USnStorageComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h" // StopMovementImmediately (unstuck)
#include "GameFramework/GameModeBase.h" // FindPlayerStart (unstuck's spawn point)
#include "GameFramework/GameStateBase.h" // PlayerArray (the profile-ID collision check)
#include "GameplayTagContainer.h"
#include "InputActionValue.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SnPlayerState.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Game/SnSettings.h"
#include "UI/WidgetController/SnCombatWidgetController.h"
#include "UI/Widget/SnChatInputWidget.h"
#include "UI/Widget/SnUserWidget.h"
#include "UI/WidgetController/USnInventoryWidgetController.h"
#include "UI/WidgetController/SnAbilityWidgetController.h"

namespace
{
	/** The player already playing in this room under ProfileID (excluding
	 *  Ignore), or null. The host files characters by that string, so two
	 *  players sharing one would write over each other's character - see
	 *  ASnPlayerController::ServerSetPlayerName. */
	const ASnPlayerState* FindPlayerUsingProfileID(const UWorld* World, const FString& ProfileID, const APlayerState* Ignore)
	{
		if (!World || ProfileID.IsEmpty())
		{
			return nullptr;
		}

		const AGameStateBase* GameState = World->GetGameState();
		if (!GameState)
		{
			return nullptr;
		}

		for (const TObjectPtr<APlayerState>& Entry : GameState->PlayerArray)
		{
			const ASnPlayerState* Other = Cast<ASnPlayerState>(Entry.Get());
			if (Other && Other != Ignore && Other->GetProfileID() == ProfileID)
			{
				return Other;
			}
		}
		return nullptr;
	}
}

ASnPlayerController::ASnPlayerController()
{
	// Development console commands (GiveItem / ListItems) live in the cheat
	// manager; the class is set here so console input is recognized in every
	// net mode (editor PIE / Development builds).
	CheatClass = USnCheatManager::StaticClass();
}

USnCombatWidgetController* ASnPlayerController::GetCombatWidgetController()
{
	if (!CombatWidgetController)
	{
		const UClass* ControllerClass = CombatWidgetControllerClass.Get();
		if (!ControllerClass)
		{
			ControllerClass = USnCombatWidgetController::StaticClass();
		}

		CombatWidgetController = NewObject<USnCombatWidgetController>(this, ControllerClass);
		CombatWidgetController->Initialize(this);
	}
	return CombatWidgetController;
}

USnAbilityWidgetController* ASnPlayerController::GetAbilityWidgetController()
{
	if (!AbilityWidgetController)
	{
		const UClass* ControllerClass = AbilityWidgetControllerClass.Get();
		if (!ControllerClass)
		{
			ControllerClass = USnAbilityWidgetController::StaticClass();
		}

		AbilityWidgetController = NewObject<USnAbilityWidgetController>(this, ControllerClass);
		AbilityWidgetController->Initialize(this);
	}
	return AbilityWidgetController;
}

void ASnPlayerController::TryInitializeUI()
{
	// Readiness gate: this controller + its player state. The player state
	// reaches a client on its own channel (the pawn carries a separate copy of
	// it) and is ordered against neither the HUD's spawn nor the pawn's
	// notifications, so every event that can complete the pair calls this again
	// instead of assuming it is the last one - the "call the check from
	// everywhere" safety net Lyra builds around CheckDefaultInitialization.
	// Whichever event lands last does the work; the rest are no-ops.
	if (!IsUIReady())
	{
		return;
	}

	// The HUD may not exist yet - a client's HUD is spawned by ClientSetHUD,
	// which can land after the player state. Its own BeginPlay runs this again.
	if (ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD()))
	{
		SnHUD->InitializeOverlay(this, GetPlayerState<ASnPlayerState>());
	}
}

bool ASnPlayerController::IsUIReady() const
{
	// A SnPlayerState, not just any player state: the overlay's widget
	// controllers read the ASC + attribute set off it, and both are subobjects
	// of the state - so the state is the whole readiness condition.
	return Cast<ASnPlayerState>(PlayerState) != nullptr;
}

void ASnPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Client only (the server assigns its player state directly, so no rep
	// notify). One of the readiness events for the gate above: when the player
	// state is the last piece to arrive, everything else has already tried and
	// nothing would ever call the gate again - which left the joining guest
	// with no HUD until the next level travel.
	TryInitializeUI();
}

void ASnPlayerController::SetDifficulty_Implementation(FName InDifficulty)
{
	// Session-scoped (RoR2): the tier lives on the GameState, not the player
	// state - every player in the run shares it, and it resets with the run.
	if (ASnGameState* GameState = GetWorld()->GetGameState<ASnGameState>())
	{
		GameState->SetDifficulty(InDifficulty);
	}
}

void ASnPlayerController::RequestTravel_Implementation(const FString& LevelName)
{
	// Server-authoritative travel with a client barrier: snapshot + autosave
	// first (the host's save is the room's save), then let every remote
	// client finish its local teardown before the world switches. Without the
	// barrier a client still mid-frame in the old level can miss the travel
	// notice and get stranded.
	if (LevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController::RequestTravel: empty level name"));
		return;
	}

	// One canonical answer to "is this the menu level?": the caller is usually a
	// blueprint passing the bare map name ("Map_1") while the settings hold a
	// package path, and comparing those two strings directly said NO to exactly
	// that case (see USnSettings::IsMainMenuLevel - it normalizes both sides).
	const bool bLeavingForMenu = USnSettings::IsMainMenuLevel(LevelName);

	// Travel on the settings' own path. The bare map name answers the question
	// above correctly, but it is not something the level switch should be handed.
	const FString MenuLevelName = USnSettings::Get()->GetMainMenuLevelName();
	const FString TravelTarget = (bLeavingForMenu && !MenuLevelName.IsEmpty()) ? MenuLevelName : LevelName;

	// A guest asking for the main menu is asking to leave the room, not to drag
	// everyone into the menu level: only that machine goes home, the room keeps
	// running. IsLocalPlayerController is the engine's own client test in this
	// situation (AGameSession::ReturnToMainMenuHost, ProcessClientTravel) - the
	// host's request comes from a local controller and falls through below,
	// taking the whole room with it.
	if (bLeavingForMenu && !IsLocalPlayerController())
	{
		ClientPrepareTravel(TravelTarget);
		return;
	}

	if (bLeavingForMenu)
	{
		// Leaving for the main menu ends the session: drop the pause and any
		// open window HERE in the old world - the menu level's BeginPlay skips
		// the in-session teardown (the travel clears bInSession), so leftovers
		// would sit on the menu level ("UI stuck open", and the viewport's
		// UI-only input state with them). Remote clients do the same teardown
		// in ClientPrepareTravel.
		if (bMenuPauseEngaged)
		{
			bMenuPauseEngaged = false;
			UGameplayStatics::SetGamePaused(this, /*bPaused=*/false);
		}
		CloseAllWindowsAndApplyInputMode();

		// Dissolve the room before the travel: this clears bIsHosting right
		// away, so ServerTravelNow does not append ?listen and the menu level
		// cannot come up as a listen server advertising a room nobody is in.
		// The guests are told to leave in the loop below - each one tears its
		// own session down and travels itself home, then confirms, so the
		// barrier still holds the host back until everyone is on their way.
		if (USnLobbySubsystem* Lobby = GetGameInstance() ? GetGameInstance()->GetSubsystem<USnLobbySubsystem>() : nullptr)
		{
			Lobby->DestroyLobby();
		}
	}

	if (USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		GI->PrepareServerTravel(TravelTarget);
	}

	// Ask every other controller to prepare, counting them as we go. The count
	// is handed to the GameInstance together with the level: the confirmations
	// come back on EACH client's own controller instance, so the barrier state
	// has to live somewhere every controller can reach (see BeginTravelBarrier).
	int32 RemoteClientCount = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (PC && PC != this)
		{
			if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(PC))
			{
				SnPC->ClientPrepareTravel(TravelTarget);
				++RemoteClientCount;
			}
		}
	}

	if (USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		// Arms the timeout and travels right away when there is nobody to wait
		// for (standalone), so single-player keeps its instant travel.
		GI->BeginTravelBarrier(TravelTarget, RemoteClientCount);
	}
}

void ASnPlayerController::ClientPrepareTravel_Implementation(const FString& LevelName)
{
	// The main menu is the way out of the session, not another level: this
	// guest tears its own session down and travels itself home. It confirms
	// right after, which is what lets the host dissolve the room safely - the
	// host's own travel (no ?listen) drops whatever is still connected, so it
	// must not happen until every guest is on its way. The confirmation is a
	// no-op when this guest asked for the menu itself (no barrier pending).
	if (USnSettings::IsMainMenuLevel(LevelName))
	{
		LeaveSessionLocally();
		ServerTravelReady();
		return;
	}

	// Nothing to write here: this machine is a guest, and the room's roster -
	// this player's character included - is the HOST's property (it captured
	// every player before it started the travel, and files them all in its own
	// save). A guest's machine never writes a save file of its own.

	// Local teardown before the world switch: the viewport client's UI-only
	// input state outlives the level, so an open window here would leave this
	// client unable to move in the next world (see the header). The widgets
	// themselves are removed by UMG on world cleanup - the input state is not.
	CloseAllWindowsAndApplyInputMode();

	ServerTravelReady();
}

void ASnPlayerController::ServerTravelReady_Implementation()
{
	// The pending travel is tracked by the GameInstance: this call lands on the
	// confirming client's own controller, so asking "is a travel pending here?"
	// always answered no and the handshake never completed.
	if (USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		GI->ConfirmTravel(this);
	}
}

void ASnPlayerController::CloseOnlineSession()
{
	// The session object belongs to the GameInstance (it must outlive level
	// travel), so it is fetched, never cached.
	USnLobbySubsystem* Lobby = GetGameInstance() ? GetGameInstance()->GetSubsystem<USnLobbySubsystem>() : nullptr;
	if (!Lobby || !Lobby->IsInLobby())
	{
		return;
	}

	// The role decides the scope of the exit. The host's version rides the
	// normal travel request, which already carries the client barrier: every
	// guest is home before the host's own travel drops the connection. Read the
	// role FIRST - DestroyLobby clears bIsHosting on its way out.
	if (Lobby->IsHosting())
	{
		RequestTravel(USnSettings::Get()->GetMainMenuLevelName());
		return;
	}

	LeaveSessionLocally();
}

void ASnPlayerController::LeaveSessionLocally()
{
	const FString MenuLevel = USnSettings::Get()->GetMainMenuLevelName();
	if (MenuLevel.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController::LeaveSessionLocally: USnSettings::MainMenuLevel is not set"));
		return;
	}

	// Window teardown first, same as any travel: leftover windows would sit on
	// the menu ("UI stuck open") and leave the menu screen on the viewport's
	// UI-only input state. The call also converges the pause pairing.
	if (bMenuPauseEngaged)
	{
		bMenuPauseEngaged = false;
		UGameplayStatics::SetGamePaused(this, /*bPaused=*/false);
	}
	CloseAllWindowsAndApplyInputMode();

	// This machine's session record goes now, before the travel: a room left
	// open keeps showing up in LAN searches with nobody behind it, and the
	// stale session would block CreateLobby back in the menu. The destroy is
	// asynchronous, but the subsystem outlives the level, so its callback lands
	// wherever this machine ends up.
	if (USnLobbySubsystem* Lobby = GetGameInstance() ? GetGameInstance()->GetSubsystem<USnLobbySubsystem>() : nullptr)
	{
		Lobby->DestroyLobby();
	}

	// Nothing is written on the way out. This machine is a guest leaving someone
	// else's room: the character it played here lives in that host's save, and
	// came from there when this machine joined. The host notices the departure
	// natively and saves the roster then (see USnGameInstance::
	// SaveDepartedCharacter) - quitting the game instead of going to the menu
	// reaches the same place by the same path, which is the point of keeping the
	// write on the host's side.

	// The menu level is never inside a session. The server-side path clears
	// this in ServerTravelNow, but this machine travels itself - without it the
	// menu's BeginPlay would run the in-session input teardown and leave the
	// menu on GameOnly (unclickable).
	if (USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		GI->ClearInSession();
	}

	// Absolute local travel: this machine loads the menu itself, no server
	// involvement - the host just sees the connection close and cleans up
	// natively.
	ClientTravel(MenuLevel, TRAVEL_Absolute);
}

void ASnPlayerController::ServerSetPlayerName_Implementation(const FString& PlayerName)
{
	// Trim + length cap: the name shows in the lobby list and the HUD.
	FString Clean = PlayerName.TrimStartAndEnd().Left(16);
	if (Clean.IsEmpty())
	{
		return;
	}
	if (ASnPlayerState* SnPlayerState = GetPlayerState<ASnPlayerState>())
	{
		SnPlayerState->SetPlayerName(Clean);

		// The same string doubles as the profile ID: it is the key the host files
		// this player's character under (the GameInstance reads it server-side),
		// and the key that entry is looked up by in every later world.
		// Whitelisted exactly like the identifier it becomes - an ID that fails
		// the check is logged and leaves the profile empty, and the character
		// then simply does not follow this player across levels (they keep
		// playing - see GetSessionKey).
		if (!USnGameInstance::IsValidPlayerID(Clean))
		{
			UE_LOG(LogTemp, Warning, TEXT("ServerSetPlayerName: '%s' is not a valid profile ID - this player's character will not be carried across travels"), *Clean);
			return;
		}

		// One ID, one character - so an ID that is already in this room is
		// refused rather than shared. Two players under one ID would write to
		// the same roster entry, and the second one's capture would silently
		// become the first one's character: their bag, their levels, their
		// wallet. The refused player keeps playing, but unidentified: nothing is
		// filed under their name and nothing follows them across levels.
		if (const APlayerState* TakenBy = FindPlayerUsingProfileID(GetWorld(), Clean, SnPlayerState))
		{
			UE_LOG(LogTemp, Warning, TEXT("ServerSetPlayerName: '%s' is already taken in this room by '%s' - this player stays unidentified and nothing will be saved for them"),
				*Clean, *TakenBy->GetPlayerName());
			// The client cannot see the refusal (it is a server-side decision),
			// so say it out loud on the HUD - otherwise the only symptom is
			// silently losing a session's progress.
			ClientMessage(FString::Printf(TEXT("ID '%s' is already in use in this room - your progress will not be saved."), *Clean));
			return;
		}

		SnPlayerState->SetProfileID(Clean);
	}
}

void ASnPlayerController::ClientShowXPNumber_Implementation(const FVector& WorldLocation, float Amount)
{
	// Screen-space feedback for the absorber only: the HUD projects the world
	// location locally (the shared damage-number pool).
	if (ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD()))
	{
		SnHUD->ShowXPNumber(WorldLocation, Amount);
	}
}

void ASnPlayerController::ServerGiveItem_Implementation(FName ItemRowName, int32 Count)
{
	ASnPlayerState* SnPlayerState = GetPlayerState<ASnPlayerState>();
	if (!SnPlayerState || !SnPlayerState->GetInventoryComponent())
	{
		ClientMessage(TEXT("GiveItem: 玩家状态或背包组件不可用"));
		return;
	}

	const int32 Added = SnPlayerState->GetInventoryComponent()->AddItem(ItemRowName, Count);
	if (Added > 0)
	{
		ClientMessage(FString::Printf(TEXT("GiveItem: 获得 %d 个 '%s'"), Added, *ItemRowName.ToString()));
	}
	else
	{
		ClientMessage(FString::Printf(TEXT("GiveItem: 背包已满，未能添加 '%s'"), *ItemRowName.ToString()));
	}
}

void ASnPlayerController::ServerGiveGold_Implementation(int64 Amount)
{
	ASnPlayerState* SnPlayerState = GetPlayerState<ASnPlayerState>();
	if (!SnPlayerState || !SnPlayerState->GetInventoryComponent())
	{
		ClientMessage(TEXT("GiveGold: 玩家状态或背包组件不可用"));
		return;
	}

	USnInventoryComponent* Inventory = SnPlayerState->GetInventoryComponent();
	const int64 Before = Inventory->GetGold();

	// AddGold owns the clamping (0 .. MaxGold) and the change broadcast, so the
	// printed after-value is what the ledger - and every client - really holds.
	Inventory->AddGold(Amount);

	ClientMessage(FString::Printf(TEXT("GiveGold: 金币 %lld -> %lld"), Before, Inventory->GetGold()));
}

void ASnPlayerController::ServerPlaceDevice_Implementation(FName DeviceID)
{
	// The record and the spawn are both server work, so this runs here rather than
	// on the client that typed the command. On a remote guest the aim the server
	// holds is the one the movement RPCs sent - the same source USnGA_Build reads
	// for a remote player, so a guest places where it is actually looking.
	APawn* MyPawn = GetPawn();
	UWorld* World = GetWorld();
	USnGameInstance* GameInstance = World ? World->GetGameInstance<USnGameInstance>() : nullptr;
	if (!MyPawn || !GameInstance)
	{
		ClientMessage(TEXT("PlaceDevice: 没有可用的角色或 GameInstance"));
		return;
	}

	// Same two calls USnGA_Build makes, with the shared defaults (the cheat has no
	// ability instance to read blueprint overrides from). Row lookup happens inside
	// AddBuiltDevice; the row check here is only so the failure is readable.
	if (!USnBuildLibrary::FindBuildDeviceRow(DeviceID))
	{
		ClientMessage(FString::Printf(TEXT("PlaceDevice: 未知设备 ID '%s'（DT_BuildDevice 的行名）"), *DeviceID.ToString()));
		return;
	}

	FVector Location;
	FRotator Rotation;
	if (!USnBuildLibrary::ComputePlacementPoint(MyPawn, this, USnBuildLibrary::DefaultPlacementRange, Location, Rotation))
	{
		ClientMessage(TEXT("PlaceDevice: 视线前方没有落点（瞄准地面再试）"));
		return;
	}
	if (!USnBuildLibrary::IsPlacementValid(MyPawn, Location, USnBuildLibrary::DefaultMinDistance))
	{
		ClientMessage(FString::Printf(TEXT("PlaceDevice: 落点太近（需离角色 ≥ %.0f cm）"), USnBuildLibrary::DefaultMinDistance));
		return;
	}

	if (AActor* Device = GameInstance->AddBuiltDevice(DeviceID, Location, Rotation))
	{
		ClientMessage(FString::Printf(TEXT("PlaceDevice: 已放置 '%s' @ (%.0f, %.0f, %.0f)"),
			*DeviceID.ToString(), Location.X, Location.Y, Location.Z));
	}
	else
	{
		// The row exists (checked above), so this is a row without a DeviceClass.
		ClientMessage(FString::Printf(TEXT("PlaceDevice: '%s' 无法生成（行的 DeviceClass 为空）"), *DeviceID.ToString()));
	}
}

void ASnPlayerController::ServerExecuteAction_Implementation(AActor* Target)
{
	// Server-authoritative re-check: the target must still exist and be
	// interactable (pooled pickups have bEnabled=false) - guards double-picks
	// and stale clicks. The action class is re-resolved from the target's row
	// on the server (never taken from the client call).
	if (!Target)
	{
		return;
	}

	UInteractableComponent* Interactable = Target->FindComponentByClass<UInteractableComponent>();
	if (!Interactable || !Interactable->bEnabled || Interactable->InteractRowName.IsNone())
	{
		return;
	}

	const FSnInteractionRow* Row = FindInteractionRow(Target);
	if (!Row || !Row->ActionClass)
	{
		return;
	}

	// Same instantiation path as the local ExecuteInteraction: stateless, per
	// execution, GC cleans up. The action's Execute runs here on the server,
	// so world mutations (AddItem, pickup retirement) hit the authoritative
	// state and replicate to everyone.
	UInteractionAction* Action = NewObject<UInteractionAction>(this, Row->ActionClass);
	Action->ConfigWidgetClass = Row->ConfigWidgetClass;
	Action->Execute(this, Target);
}

void ASnPlayerController::RequestUnstuck()
{
	// Straight to the server: the teleport writes the pawn's transform, and the
	// pawn is server-authoritative. On a listen host the RPC body runs inline,
	// so the host's own menu button takes exactly the same path as a guest's.
	ServerUnstuck();
}

void ASnPlayerController::ServerUnstuck_Implementation()
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		// Dead / mid-travel / not possessed yet: there is no body to move. The
		// death return (ASnGameModeBase) is already taking this player home.
		return;
	}

	// Cooldown gate: the request comes from the client, but the pawn and this
	// clock are both server-owned, so the gate lives here (UnstuckCooldown).
	if (UnstuckCooldown > 0.f)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (LastUnstuckTime >= 0.f && Now - LastUnstuckTime < UnstuckCooldown)
		{
			ClientMessage(FString::Printf(TEXT("脱离卡死：冷却中（还有 %.1f 秒）"),
				UnstuckCooldown - (Now - LastUnstuckTime)));
			return;
		}
	}

	// Where a fresh spawn would go: the GameMode owns the spawn point search
	// (FindPlayerStart, BP overrides included), so unstuck can never drift from
	// what RestartPlayer does with the same level.
	AGameModeBase* GameMode = GetWorld()->GetAuthGameMode<AGameModeBase>();
	AActor* PlayerStartSpot = GameMode ? GameMode->FindPlayerStart(this) : nullptr;
	if (!PlayerStartSpot)
	{
		// No PlayerStart in the level: nowhere to send the player, and staying
		// put is the only honest answer - say so instead of failing silently.
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController::ServerUnstuck: 本关没有 PlayerStart，无法定位出生点"));
		ClientMessage(TEXT("脱离卡死：本关没有 PlayerStart，无法定位出生点"));
		return;
	}

	const FVector StartLocation = PlayerStartSpot->GetActorLocation();

	// Roll is dropped, Pitch and Yaw are honored. Roll is never meaningful - not
	// for a player view and not for a standing character - yet a PlayerStart picks
	// it up silently when the actor is rotated with a gizmo in the editor, and
	// nothing downstream filters it: ClientSetRotation feeds the control rotation
	// verbatim, and the camera boom (bUsePawnControlRotation) takes its world
	// rotation straight from there, so the view comes back permanently tilted
	// sideways. The engine's own respawn zeroes it for the same reason:
	// AGameModeBase::FinishRestartPlayer sets the controller rotation and then
	// clears Roll (GameModeBase.cpp:1376-1378). Pitch and Yaw stay because those
	// are deliberate level design - a start point facing down into the arena.
	FRotator StartRotation = PlayerStartSpot->GetActorRotation();
	StartRotation.Roll = 0.f;

	// Teleport with the collision check ON: AActor::TeleportTo runs the engine's
	// FindTeleportSpot, which nudges the destination to a place the capsule
	// actually fits. A capsule that is embedded right now can defeat that
	// search, and a recovery feature that silently does nothing is worse than
	// useless - so the refusal falls back to an unchecked teleport with
	// ETeleportType::TeleportPhysics (the call the pooled enemies are
	// repositioned with, SnEnemy), which moves the body out of anything.
	if (!MyPawn->TeleportTo(StartLocation, StartRotation, /*bIsATest=*/false, /*bNoCheck=*/false))
	{
		MyPawn->SetActorLocationAndRotation(StartLocation, StartRotation, /*bSweep=*/false, nullptr, ETeleportType::TeleportPhysics);
	}

	// A player who fell out of the world is usually in a long fall: the teleport
	// moves the body but leaves the velocity behind, and landing at the spawn
	// with terminal velocity reads as being fired out of the ground.
	if (ACharacter* MyCharacter = Cast<ACharacter>(MyPawn))
	{
		MyCharacter->GetCharacterMovement()->StopMovementImmediately();
	}

	// Turn the VIEW to the spawn facing - the engine's own respawn ends with the
	// same call (AGameModeBase::FinishRestartPlayer), and it matters more here
	// than there: the player was just moved across the level looking somewhere
	// arbitrary. Client RPC, so it lands on the machine that owns the camera
	// (locally on a listen host). The position half needs no RPC - the server
	// owns it, and TeleportSucceeded forces the net update out. The body is
	// untouched: FaceRotation only ever rotates a pawn that uses controller
	// rotation, and this project's free-camera character does not
	// (bUseControllerRotationYaw = false, see ASnCharacter) - the view turns,
	// the character keeps the facing its movement gives it, same as the whole
	// strike-aiming model.
	ClientSetRotation(StartRotation, /*bResetCamera=*/true);

	LastUnstuckTime = GetWorld()->GetTimeSeconds();
}

USnWindowManager* ASnPlayerController::GetWindowManager()
{
	if (!WindowManager)
	{
		WindowManager = NewObject<USnWindowManager>(this);
		WindowManager->Initialize(this);
	}
	return WindowManager;
}

void ASnPlayerController::OpenWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource)
{
	if (USnWindowManager* Manager = GetWindowManager())
	{
		Manager->OpenWindow(WidgetClass, DataSource);
	}
}

void ASnPlayerController::RaiseWindow(USnActivatableWindow* Window)
{
	if (USnWindowManager* Manager = GetWindowManager())
	{
		Manager->RaiseWindow(Window);
	}
}

bool ASnPlayerController::CloseWindow()
{
	bool bClosed = false;
	if (USnWindowManager* Manager = GetWindowManager())
	{
		bClosed = Manager->CloseWindow();
	}

	// Re-pair the single-player pauses with their windows' visibility after
	// every close: Escape on the focused window, menu buttons, the dialogue's
	// last line and travel teardown all funnel through here - neither pause
	// can outlive its window.
	UpdateMenuPause();
	UpdateDialoguePause();

	return bClosed;
}

void ASnPlayerController::CloseAllWindowsAndApplyInputMode()
{
	// Through this controller's CloseWindow, not the manager's: every close runs
	// the pause pairing above, so a menu/dialogue pause cannot outlive its
	// window just because the travel tore it down.
	while (IsConfigWindowOpen())
	{
		CloseWindow();
	}

	// The chat line is not a window but it holds the input mode the same way:
	// leaving it open across a travel would strand the next world in UI-only -
	// which is exactly the bug this function exists to prevent. Also covers the
	// client-side travel paths (they close nothing else).
	CloseChatInput();

	// GetWindowManager (lazy create) rather than the raw pointer: the arrival
	// case has no manager yet, and an empty one applies exactly the right mode -
	// no windows means game-only + hidden cursor.
	if (USnWindowManager* Manager = GetWindowManager())
	{
		Manager->ApplyInputModeNow();
	}
}

void ASnPlayerController::UpdateMenuPause()
{
	// Release the pause exactly when the main menu is gone (escaped, Continue,
	// or torn down by a travel). See the flag's comment in the header - the
	// pairing lives here so no caller can forget the release half.
	if (bMenuPauseEngaged && !IsWindowOpen(MainMenuWidgetClass))
	{
		bMenuPauseEngaged = false;
		UGameplayStatics::SetGamePaused(this, /*bPaused=*/false);
	}
}

void ASnPlayerController::UpdateDialoguePause()
{
	// Release the dialogue pause exactly when the popup is gone: the last
	// line closed it, Escape skipped it, or a travel tore it down. Same
	// pairing shape as UpdateMenuPause - the pause never outlives the window.
	if (bDialoguePauseEngaged && !IsWindowOpen(DialogueWidgetClass))
	{
		bDialoguePauseEngaged = false;
		UGameplayStatics::SetGamePaused(this, /*bPaused=*/false);
	}
}

void ASnPlayerController::ToggleWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource)
{
	if (USnWindowManager* Manager = GetWindowManager())
	{
		Manager->ToggleWindow(WidgetClass, DataSource);
	}
}

bool ASnPlayerController::IsWindowOpen(TSubclassOf<USnActivatableWindow> WidgetClass) const
{
	// Not lazy-created here: a const query must not mutate; a manager that was
	// never created simply means no window was ever opened.
	return WindowManager ? WindowManager->IsWindowOpen(WidgetClass) : false;
}

void ASnPlayerController::ToggleAttributePanel()
{
	if (!AttributePanelWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController: AttributePanelWidgetClass 未配置（PC 默认值里设成 WBP_AttributePanel）"));
		return;
	}

	// A regular window like the inventory: it coexists with other open windows
	// instead of covering them. The resident combat widget controller is the
	// data source - the same values as the HUD's combat widgets.
	ToggleWindow(AttributePanelWidgetClass, GetCombatWidgetController());
}

bool ASnPlayerController::IsAttributePanelOpen() const
{
	return AttributePanelWidgetClass ? IsWindowOpen(AttributePanelWidgetClass) : false;
}

void ASnPlayerController::ToggleCheatPanel()
{
	if (!CheatPanelWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController: CheatPanelWidgetClass 未配置（BP_SnPlayerController 默认值里设成 WBP_CheatPanel）"));
		return;
	}

	// Dev-only window. No data source of its own: the PC is injected so the
	// FIRST open fires WidgetControllerSet (the widget builds its item list
	// there) - a null source would hit the same-source early-out and the
	// event would never run.
	ToggleWindow(CheatPanelWidgetClass, this);
}

void ASnPlayerController::ToggleMainMenu()
{
	if (!MainMenuWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController: MainMenuWidgetClass 未配置（BP_SnPlayerController 默认值里设成 WBP_MainMenu）"));
		return;
	}

	// A resident window like any other; the PC is the data source (no
	// controller of its own - the source exists so the first open fires
	// WidgetControllerSet). The intended entry is Esc with no window open
	// (HandleEscape); opening from elsewhere would stack it like any window.
	ToggleWindow(MainMenuWidgetClass, this);

	if (IsWindowOpen(MainMenuWidgetClass))
	{
		// Single player pauses so the world freezes behind the menu; LAN
		// sessions never pause (SetGamePaused freezes every sim - a networked
		// trap). The flag records that THIS PC paused, so the release in
		// UpdateMenuPause can be exact.
		if (GetNetMode() == NM_Standalone)
		{
			UGameplayStatics::SetGamePaused(this, /*bPaused=*/true);
			bMenuPauseEngaged = true;
		}
	}
	else
	{
		// Toggled closed (the menu was open): release the paired pause.
		UpdateMenuPause();
	}
}

void ASnPlayerController::OpenSettingsWindow()
{
	if (!SettingsWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController: SettingsWidgetClass 未配置（BP_SnPlayerController 默认值里设成 WBP_Settings）"));
		return;
	}

	// Stacked above the main menu - a regular window: Escape on it closes
	// back to the menu (the topmost). The pause stays engaged while the main
	// menu stays visible (the settings window never pauses by itself).
	OpenWindow(SettingsWidgetClass, this);
}

void ASnPlayerController::PlayDialogue(FName FirstRow, bool bPauseGame)
{
	if (FirstRow.IsNone())
	{
		return;
	}

	if (!DialogueWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayDialogue: DialogueWidgetClass 未配置（BP_SnPlayerController 默认值里设成 WBP_Dialogue）"));
		return;
	}

	// The first row must exist - the window only opens over a real line (the
	// controller would otherwise show an empty box).
	UDataTable* Table = USnSettings::Get()->GetDialogueDataTable();
	if (!Table || !Table->FindRow<FSnDialogueRow>(FirstRow, TEXT("PlayDialogue")))
	{
		UE_LOG(LogTemp, Warning, TEXT("PlayDialogue: DialogueDataTable 未配置或找不到起始行 %s"), *FirstRow.ToString());
		return;
	}

	// A dialogue already on screen restarts from the new first row. (The intro
	// tutorial never re-triggers - milestone-guarded - but future scripts can
	// chain dialogues, and the resident window would otherwise keep the stale
	// controller.)
	if (IsWindowOpen(DialogueWidgetClass))
	{
		CloseWindow();
	}

	// Fresh transient controller per dialogue, same as the inventory/crafting
	// windows: Start sets the first row, the window pulls the rest.
	USnDialogueController* Controller = NewObject<USnDialogueController>(this);
	Controller->Initialize(this);
	Controller->Start(FirstRow);

	OpenWindow(DialogueWidgetClass, Controller);

	// Tutorial-style dialogues pause the world behind the popup - single
	// player only, the LAN discipline is absolute (SetGamePaused freezes
	// every sim). The release is paired with the popup's visibility in
	// UpdateDialoguePause (funneled from CloseWindow), so every close path -
	// last line, Escape, travel teardown - restores the game exactly.
	if (bPauseGame && GetNetMode() == NM_Standalone)
	{
		UGameplayStatics::SetGamePaused(this, /*bPaused=*/true);
		bDialoguePauseEngaged = true;
	}
}

void ASnPlayerController::ToggleInventory()
{
	if (!InventoryWidgetClass)
	{
		return;
	}

	// Multiple windows coexist: toggling only the inventory - the attribute
	// panel stays up. The controller is created per open (fresh transient, no
	// stale bindings); the close path never uses it.
	USnWindowManager* Manager = GetWindowManager();
	if (!Manager)
	{
		return;
	}
	if (Manager->IsWindowOpen(InventoryWidgetClass))
	{
		Manager->ToggleWindow(InventoryWidgetClass, nullptr);
		return;
	}

	USnInventoryWidgetController* Controller = NewObject<USnInventoryWidgetController>(this);
	Controller->Initialize(this);

	Manager->ToggleWindow(InventoryWidgetClass, Controller);
}

void ASnPlayerController::OpenStorageWindow(USnStorageComponent* Storage)
{
	if (!Storage || !InventoryWidgetClass)
	{
		return;
	}

	USnInventoryWidgetController* Controller = NewObject<USnInventoryWidgetController>(this);
	Controller->Initialize(this);
	Controller->SetStorage(Storage);

	OpenWindow(InventoryWidgetClass, Controller);
}

USnInventoryWidgetController* ASnPlayerController::CreateInventoryWidgetController()
{
	// Fresh per call, never cached: windows are resident instances, so a re-open
	// gets a new source, WidgetControllerSet fires again and BP re-binds - no
	// controller (or its bindings) outlives the window it was made for.
	// Initialize hands over PC/PS/GAS and runs BindCallbacks.
	USnInventoryWidgetController* Controller = NewObject<USnInventoryWidgetController>(this);
	Controller->Initialize(this);
	return Controller;
}

void ASnPlayerController::OpenCraftingWindow(TSubclassOf<UUserWidget> WidgetClass)
{
	if (!WidgetClass || !WidgetClass->IsChildOf(USnActivatableWindow::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenCraftingWindow: 窗口类 %s 不是 USnActivatableWindow 子类（请 reparent 到 SnActivatableWindow）"), WidgetClass ? *WidgetClass->GetName() : TEXT("None"));
		return;
	}

	// The crafting window's data source is the bag (material availability
	// refreshes through OnInventoryChanged); the widget class stays table-driven
	// via the interaction row's ConfigWidgetClass.
	OpenWindow(TSubclassOf<USnActivatableWindow>(WidgetClass.Get()), CreateInventoryWidgetController());
}

void ASnPlayerController::OpenConfigWindow(TSubclassOf<UUserWidget> WidgetClass, UObject* DataSource)
{
	if (!WidgetClass || !WidgetClass->IsChildOf(USnActivatableWindow::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenConfigWindow: 窗口类 %s 不是 USnActivatableWindow 子类（请 reparent 到 SnActivatableWindow）"), WidgetClass ? *WidgetClass->GetName() : TEXT("None"));
		return;
	}
	OpenWindow(TSubclassOf<USnActivatableWindow>(WidgetClass.Get()), DataSource);
}

bool ASnPlayerController::CloseConfigWindow()
{
	return CloseWindow();
}

bool ASnPlayerController::IsConfigWindowOpen() const
{
	// Not lazy-created here (const query); a missing manager means no window
	// was ever opened.
	return WindowManager ? WindowManager->IsAnyWindowOpen() : false;
}

void ASnPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// A traveled-into world must start clean: unpause (a pause set on the menu
	// can survive the load) and re-sync the window/input state. This controller
	// is brand new - non-seamless travel recreates it - so its window manager
	// holds nothing, but the PREVIOUS world may have left the game viewport
	// client in UI-only (a window was open when the travel started: the menu ->
	// Continue Game path) and nothing else would ever take the mouse capture and
	// the Slate focus back. This call is that repair - without it WASD and
	// mouse-look are dead for the whole level ("entered the run map, cannot
	// move"). Only inside a session - the main menu keeps its own widget-driven
	// setup.
	if (USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		if (GI->IsInSession())
		{
			UGameplayStatics::SetGamePaused(this, /*bPaused=*/false);
			// The in-session BeginPlay releases the pause unconditionally, so
			// the paired flags must follow (a stale flag would release nothing
			// and linger into the next menu/dialogue open).
			bMenuPauseEngaged = false;
			bDialoguePauseEngaged = false;
			CloseAllWindowsAndApplyInputMode();
		}
	}
}

void ASnPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInput)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController: InputComponent is not a UEnhancedInputComponent. Check Default Input Classes in Project Settings."));
		return;
	}

	EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASnPlayerController::HandleMove);
	EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASnPlayerController::HandleLook);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleJumpStarted);
	EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &ASnPlayerController::HandleJumpCompleted);
	EnhancedInput->BindAction(AttributePanelAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleToggleAttributePanel);
	EnhancedInput->BindAction(MouseClickAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleMouseClick);
	EnhancedInput->BindAction(InteractAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleInteract);
	EnhancedInput->BindAction(MouseWheelAction, ETriggerEvent::Triggered, this, &ASnPlayerController::HandleMouseWheel);
	EnhancedInput->BindAction(InventoryAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleToggleInventory);
	EnhancedInput->BindAction(EscapeAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleEscape);

	// Same guard as the cheat panel: an unset action is never bound, so a
	// project that has not mapped chat yet starts without an Enter handler
	// instead of asserting on a null action.
	if (ChatInputAction)
	{
		EnhancedInput->BindAction(ChatInputAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleChatPressed);
	}

	// Developer cheat panel (see CheatPanelAction): an unset action is never
	// bound, so the debug entry point can be left off in shipped builds.
	if (CheatPanelAction)
	{
		EnhancedInput->BindAction(CheatPanelAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleToggleCheatPanel);
	}

	// Hotbar: the numeric keys select their slot (index bound as the handler's
	// payload parameter), right click uses the selection. Both are client-safe
	// wrappers on the inventory component - the RPCs execute on the server and
	// replicate back.
	for (int32 SlotIndex = 0; SlotIndex < HotbarSlotActions.Num() && SlotIndex < USnInventoryComponent::HotbarSize; ++SlotIndex)
	{
		if (HotbarSlotActions[SlotIndex])
		{
			EnhancedInput->BindAction(HotbarSlotActions[SlotIndex], ETriggerEvent::Started, this, &ASnPlayerController::HandleSelectHotbarSlot, SlotIndex);
		}
	}
	EnhancedInput->BindAction(UseItemAction, ETriggerEvent::Started, this, &ASnPlayerController::HandleUseHotbarItem);
}

void ASnPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// Server-side possession: the client never runs OnPossess (its pawn arrives
	// by replication, handled in OnRep_Pawn) - both paths land in the shared
	// setup below.
	SetupLocalPlayerPawn(InPawn);
}

void ASnPlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	// Client possession: without this hook the client gets no Enhanced Input
	// mappings and no interaction subscription - WASD/mouse do nothing and the
	// F-key interaction never works. Re-fires on respawn like OnPossess does.
	SetupLocalPlayerPawn(GetPawn());
}

void ASnPlayerController::SetupLocalPlayerPawn(APawn* InPawn)
{
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(InputMappingContext, /*Priority=*/0);
	}

	// Cache the pawn's interaction component and subscribe to its broadcasts
	// (re-subscribes cleanly on respawn).
	if (InteractionComponent)
	{
		InteractionComponent->OnInteractablesUpdated.RemoveDynamic(this, &ASnPlayerController::HandleInteractablesUpdated);
	}

	InteractionComponent = InPawn ? InPawn->FindComponentByClass<UPlayerInteractionComponent>() : nullptr;
	if (InteractionComponent)
	{
		InteractionComponent->OnInteractablesUpdated.AddDynamic(this, &ASnPlayerController::HandleInteractablesUpdated);
	}

	// Identity report: JoinSession carries no name, so the controller reports
	// this machine's player ID once possessed (runs on both possession paths -
	// the host's RPC executes locally, a guest's hops to the server).
	// Idempotent, so respawns just re-send the same string.
	//
	// Gated on this machine really driving the controller. On a listen server
	// OnPossess also runs for every remote guest's PC - and a Server RPC invoked
	// on the server executes RIGHT THERE rather than being sent anywhere
	// (AActor::GetFunctionCallspace, "server calling Server function" returns
	// Local with no Remote bit, so nothing replicates). Ungated, the host would
	// stamp its own ID and display name onto each guest's player state as that
	// guest possesses, and the guest's character would then be captured under
	// the host's profile. GetLocalPlayer() is the engine's own discriminator:
	// remote PCs on a listen server have none, the listen host and true clients
	// do (PlayerController.cpp:330).
	//
	// Sent outside a lobby too: the same call is what gives the server a
	// ProfileID for this player, which is the key their character is captured
	// under when the room travels. Standalone and PIE sessions report their ID
	// the same way, so there is one identity path everywhere. An empty ID is
	// skipped rather than sent - a machine that never bound one has nothing to
	// report, and the server keeps an empty profile for it.
	if (GetLocalPlayer() != nullptr)
	{
		if (const USnGameInstance* GI = GetGameInstance<USnGameInstance>())
		{
			const FString PlayerID = GI->GetPlayerID();
			if (!PlayerID.IsEmpty())
			{
				ServerSetPlayerName(PlayerID);
			}
		}
	}
}

void ASnPlayerController::HandleMove(const FInputActionValue& Value)
{
	const FVector2D MoveVector = Value.Get<FVector2D>();

	// Camera-relative movement: forward/back on Y, strafe on X.
	const FRotator YawRotation(0.f, GetControlRotation().Yaw, 0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

	if (APawn* ControlledPawn = GetPawn())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, MoveVector.Y);
		ControlledPawn->AddMovementInput(RightDirection, MoveVector.X);
	}
}

void ASnPlayerController::HandleLook(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();

	AddYawInput(LookVector.X);
	AddPitchInput(LookVector.Y);
}

void ASnPlayerController::HandleJumpStarted(const FInputActionValue& Value)
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->Jump();
	}
}

void ASnPlayerController::HandleJumpCompleted(const FInputActionValue& Value)
{
	if (ACharacter* ControlledCharacter = GetCharacter())
	{
		ControlledCharacter->StopJumping();
	}
}

void ASnPlayerController::HandleToggleAttributePanel(const FInputActionValue& Value)
{
	ToggleAttributePanel();
}

void ASnPlayerController::HandleToggleCheatPanel(const FInputActionValue& Value)
{
	// Same entry point as the ItemPanel console command (USnCheatManager).
	// Local UI: adding an item still goes through the inventory's AddItem
	// server RPC, so the panel never grants items by itself.
	ToggleCheatPanel();
}

void ASnPlayerController::HandleMouseClick(const FInputActionValue& Value)
{
	// While any window is open, UI-only mode blocks game input, so clicks never
	// reach this handler (no attack-through the UI). Window closed = attack.
	if (APawn* ControlledPawn = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn))
		{
			ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(SnGameplayTags::Ability_Attack));
		}
	}
}

void ASnPlayerController::HandleInteract(const FInputActionValue& Value)
{
	// Re-scan first so the list is current, then execute the selected target
	// (which re-checks range before acting).
	if (InteractionComponent)
	{
		InteractionComponent->ForceScan();
	}

	ExecuteInteraction();
}

void ASnPlayerController::HandleMouseWheel(const FInputActionValue& Value)
{
	const float WheelDelta = Value.Get<float>();
	if (FMath::IsNearlyZero(WheelDelta))
	{
		return;
	}
	const int32 Direction = WheelDelta > 0.f ? 1 : -1;

	// Smart split: several interaction candidates - the wheel cycles the
	// interaction selection (pickups on the ground). Zero or one candidate -
	// selection cycling is a no-op there - it cycles the hotbar selection
	// instead, so the wheel never idles.
	ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>();
	USnInventoryComponent* Inventory = SnPS ? SnPS->GetInventoryComponent() : nullptr;
	if (!InteractionComponent || InteractionComponent->GetVisibleInteractables().Num() <= 1 || !Inventory)
	{
		if (Inventory)
		{
			Inventory->CycleHotbar(Direction);
		}
		return;
	}

	InteractionComponent->CycleSelection(Direction);
}

void ASnPlayerController::HandleSelectHotbarSlot(const FInputActionValue& Value, int32 SlotIndex)
{
	// Selection is replicated session state on the inventory component: the
	// wrapper runs locally on the server (listen host) and routes through the
	// server RPC on clients. Out-of-range slots are rejected by the component.
	if (ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>())
	{
		if (USnInventoryComponent* Inventory = SnPS->GetInventoryComponent())
		{
			Inventory->SetSelectedHotbarIndex(SlotIndex);
		}
	}
}

void ASnPlayerController::HandleUseHotbarItem(const FInputActionValue& Value)
{
	// Right click: use whatever the hotbar selection holds. The component runs
	// the local cooldown gate + local UI cooldown mirror, then the
	// authoritative use RPC (the server re-checks its own per-item cooldown,
	// the bag, and applies/consumes).
	if (ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>())
	{
		if (USnInventoryComponent* Inventory = SnPS->GetInventoryComponent())
		{
			Inventory->UseSelectedHotbarItem();
		}
	}
}

void ASnPlayerController::HandleToggleInventory(const FInputActionValue& Value)
{
	ToggleInventory();
}

void ASnPlayerController::HandleEscape(const FInputActionValue& Value)
{
	// While a window is open, UI-only mode blocks game input - Escape is
	// consumed by the focused window's NativeOnKeyDown (closes the topmost).
	// This binding is the fallback for the no-window case: it opens the
	// in-game main menu (the pause menu, see ToggleMainMenu).
	if (!IsConfigWindowOpen())
	{
		ToggleMainMenu();
	}
}

void ASnPlayerController::HandleChatPressed(const FInputActionValue& Value)
{
	// Reachable only with no window and no line open: both states are UI-only,
	// which keeps this game binding from firing at all. The guard mirrors
	// HandleEscape (the same shape for the same reason) and covers the caller
	// that is not a key - the console command.
	if (!IsConfigWindowOpen())
	{
		OpenChatInput();
	}
}

void ASnPlayerController::OpenChatInput()
{
	if (bChatInputActive || IsConfigWindowOpen())
	{
		return;
	}

	ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD());
	if (!SnHUD)
	{
		return;
	}

	if (!SnHUD->ShowChatInput())
	{
		// No widget class configured: there is nothing to type into, so the
		// input mode is left alone. Locking the game input with no visible box
		// would just look like the game froze.
		return;
	}

	bChatInputActive = true;

	// Immediate, unlike the close below: this is not reacting to a key that is
	// still on its way up, and the box has to hold the keyboard before the
	// player's first character arrives.
	if (USnWindowManager* Manager = GetWindowManager())
	{
		Manager->ApplyInputModeNow();
	}
}

void ASnPlayerController::CloseChatInput()
{
	if (!bChatInputActive)
	{
		return;
	}

	// State before widget: the input mode is derived from it.
	bChatInputActive = false;
	if (ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD()))
	{
		SnHUD->HideChatInput();
	}

	// Deferred through the manager's own path, exactly like the last window
	// closing: the Enter that committed (or the Escape that cancelled) is still
	// bubbling through Slate, and switching to game-only in THIS frame would
	// hand that same key to the game's Enter binding - the line would open
	// right back up ("press Enter twice to close").
	if (USnWindowManager* Manager = GetWindowManager())
	{
		Manager->UpdateInputMode();
	}
}

void ASnPlayerController::SubmitChatText(const FString& Text)
{
	// Copy the trimmed line out before anything can clear the box: Text is a
	// reference to the input widget's own string.
	const FString Trimmed = Text.TrimStartAndEnd();

	// Close first: the typing session is over the moment Enter lands, whether
	// or not there is anything to send.
	CloseChatInput();

	if (Trimmed.IsEmpty())
	{
		// Empty Enter = cancel. Nothing is sent, nothing is logged - and the
		// server would drop it anyway, so the two exits agree.
		return;
	}

	ServerSendChatMessage(Trimmed);
}

TSharedPtr<SWidget> ASnPlayerController::GetChatFocusWidget() const
{
	ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD());
	USnChatInputWidget* Input = SnHUD ? SnHUD->GetChatInputWidget() : nullptr;
	return Input ? Input->GetFocusWidget() : nullptr;
}

void ASnPlayerController::ServerSendChatMessage_Implementation(const FString& Message)
{
	// Everything that decides whether a line is said happens HERE: the client
	// validates nothing it does not have to, and the string everyone renders is
	// the string this function accepted.
	FString Trimmed = Message.TrimStartAndEnd();
	if (Trimmed.IsEmpty())
	{
		return;
	}

	if (ChatMaxLength > 0 && Trimmed.Len() > ChatMaxLength)
	{
		Trimmed.LeftInline(ChatMaxLength);
	}

	// Rate limit on the server's clock (the unstuck gate, word for word).
	if (ChatCooldown > 0.f)
	{
		const float Now = GetWorld()->GetTimeSeconds();
		if (LastChatTime >= 0.f && Now - LastChatTime < ChatCooldown)
		{
			return;
		}
		LastChatTime = Now;
	}

	// Who said it is the pawn the multicast runs on - no need to name a speaker
	// (and no client-supplied name to distrust). No pawn = no voice: dead,
	// mid-travel or not possessed.
	ASnCharacter* Speaker = Cast<ASnCharacter>(GetPawn());
	if (!Speaker)
	{
		return;
	}

	Speaker->MulticastShowChatBubble(Trimmed);
}

void ASnPlayerController::ExecuteInteraction()
{
	if (!InteractionComponent)
	{
		return;
	}

	AActor* Target = InteractionComponent->GetSelectedTarget();
	if (!Target || !InteractionComponent->IsValidTarget(Target))
	{
		return;
	}

	const FSnInteractionRow* Row = FindInteractionRow(Target);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnPlayerController: no interaction row for %s"), *Target->GetName());
		return;
	}

	const TSubclassOf<UInteractionAction> ActionClass = Row->ActionClass;
	if (!ActionClass)
	{
		return;
	}

	// Execution location is the action's own declaration (ESnActionExecution):
	// world-modifying actions (pickups) route through the server - running them
	// locally would only mutate this client's copies (ghost inventory, the
	// pickup stays alive server-side). On the server itself (listen host) the
	// RPC call is skipped and the action runs directly below.
	if (ActionClass->GetDefaultObject<UInteractionAction>()->Execution == ESnActionExecution::Server && !HasAuthority())
	{
		ServerExecuteAction(Target);
		return;
	}

	// Actions are stateless: instantiate per execution and let GC clean up.
	UInteractionAction* Action = NewObject<UInteractionAction>(this, ActionClass);

	// The config window widget comes from the row (table-driven, nothing
	// hardcoded in the action blueprint).
	Action->ConfigWidgetClass = Row->ConfigWidgetClass;

	Action->Execute(this, Target);
}

const FSnInteractionRow* ASnPlayerController::FindInteractionRow(const AActor* Target) const
{
	if (!Target)
	{
		return nullptr;
	}

	UInteractableComponent* Interactable = Target->FindComponentByClass<UInteractableComponent>();
	if (!Interactable || Interactable->InteractRowName.IsNone())
	{
		return nullptr;
	}

	UDataTable* Table = USnSettings::Get()->GetInteractionDataTable();
	return Table ? Table->FindRow<FSnInteractionRow>(Interactable->InteractRowName, TEXT("SnPlayerController::FindInteractionRow")) : nullptr;
}

void ASnPlayerController::HandleInteractablesUpdated(TArray<AActor*> Interactables, int32 SelectedIndex)
{
	UpdateInteractionUI(Interactables, SelectedIndex);
}

void ASnPlayerController::UpdateInteractionUI(const TArray<AActor*>& Interactables, int32 SelectedIndex)
{
	if (Interactables.Num() == 0 || !InteractionComponent)
	{
		ClearInteractionUI();
		return;
	}

	// Build the transient controller: entry per visible target with its name.
	USnInteractionController* NewController = NewObject<USnInteractionController>(this);
	NewController->SelectedIndex = SelectedIndex;
	for (AActor* Target : Interactables)
	{
		FSnInteractionEntry Entry;
		Entry.Target = Target;
		// Item pickups show the item's own name + count instead of the shared
		// interaction row's generic prompt (all pickups share one row).
		if (const ASnItemPickup* Pickup = Cast<ASnItemPickup>(Target))
		{
			Entry.DisplayName = USnItemLibrary::GetStackDisplayName(Pickup->GetItemRowName(), Pickup->GetItemCount());
		}
		else if (const FSnInteractionRow* Row = FindInteractionRow(Target))
		{
			Entry.DisplayName = Row->DisplayName;
		}
		NewController->Entries.Add(Entry);
	}

	// Three-tier widget class: action override > row override > PC default.
	// The selected entry drives the class so per-action UI overrides follow
	// what the player is actually looking at.
	AActor* Focused = Interactables.IsValidIndex(SelectedIndex) ? Interactables[SelectedIndex] : Interactables[0];
	TSubclassOf<UUserWidget> WidgetClass = ResolveWidgetClass(FindInteractionRow(Focused));
	if (!WidgetClass)
	{
		return;
	}

	// The HUD owns the widget lifecycle (create / reuse / delayed teardown).
	if (ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD()))
	{
		SnHUD->ShowInteractionWidget(NewController, WidgetClass);
	}
}

TSubclassOf<UUserWidget> ASnPlayerController::ResolveWidgetClass(const FSnInteractionRow* Row) const
{
	if (Row)
	{
		if (const UClass* ActionClass = Row->ActionClass)
		{
			if (const UInteractionAction* ActionCDO = GetDefault<UInteractionAction>(ActionClass))
			{
				if (TSubclassOf<UUserWidget> Override = ActionCDO->OverrideWidgetClass.LoadSynchronous())
				{
					return Override;
				}
			}
		}

		if (const TSubclassOf<UUserWidget> Override = Row->OverrideWidgetClass)
		{
			return Override;
		}
	}

	return InteractionPromptWidgetClass;
}

void ASnPlayerController::ClearInteractionUI()
{
	if (ASnHUD* SnHUD = Cast<ASnHUD>(GetHUD()))
	{
		SnHUD->ClearInteractionWidget();
	}
}
