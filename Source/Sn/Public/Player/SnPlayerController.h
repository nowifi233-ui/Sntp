// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/UInteractionAction.h"
#include "UI/Window/SnActivatableWindow.h"
#include "UI/Window/SnWindowManager.h"
#include "SnPlayerController.generated.h"

class UInputAction;
class UInputMappingContext;
class UInteractableComponent;
class UPlayerInteractionComponent;
class UUserWidget;
class USnCombatWidgetController;
class USnAbilityWidgetController;
class USnInventoryWidgetController;
class USnStorageComponent;
class SWidget;
struct FInputActionValue;
struct FSnInteractionRow;

/**
 * Third-person player controller.
 * Binds Enhanced Input actions and forwards movement / look input to the possessed pawn.
 */
UCLASS()
class SN_API ASnPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	ASnPlayerController();

	/** Creates the combat widget controller on first access (lazy creation). */
	USnCombatWidgetController* GetCombatWidgetController();

	/** Creates the ability widget controller on first access (lazy creation).
	 *  Pull-based: skill bars / tooltips call GetAbilities when they show. */
	USnAbilityWidgetController* GetAbilityWidgetController();

	/** The window manager: owns every window (instances, Z order, input mode,
	 *  focus). Lazy-created on first access. */
	USnWindowManager* GetWindowManager();

	/** Brings the window to the top of the Z order and refocuses it (called by
	 *  the window base on unhandled mouse-downs - click-to-raise). */
	void RaiseWindow(USnActivatableWindow* Window);

	/** The player-UI readiness gate. The overlay needs THIS controller and this
	 *  controller's player state, and on a client those are two separate
	 *  replication events with no relative order guarantee - so every event that
	 *  can complete the pair calls this again, and each call is a no-op until it
	 *  is complete (whichever event lands last does the work):
	 *   - the pawn's notification, once InitAbilityActorInfo has run
	 *   - this controller's OnRep_PlayerState (the decisive client event)
	 *   - the HUD's own BeginPlay (it asks the controller instead of guessing)
	 *  Same shape as Lyra's CheckDefaultInitialization: one idempotent check,
	 *  re-run from every readiness event, because replication order cannot be
	 *  controlled. Idempotent end to end - InitializeOverlay does nothing once
	 *  the widgets exist. */
	void TryInitializeUI();

	/** Whether the pair above is complete. The ASC and the attribute set are
	 *  subobjects of the player state, so they exist the moment it does - the
	 *  state itself is the only thing worth waiting for. */
	bool IsUIReady() const;

	/** Sets the session's difficulty tier (row name in USnSettings
	 *  DifficultyDataTable), e.g. picked from the spawner menu. Server RPC:
	 *  the write lands on the server-authoritative GameState::Difficulty
	 *  (session-scoped, RoR2-style - not per-player) and replicates back. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Difficulty")
	void SetDifficulty(FName InDifficulty);

	/** Starts a level travel (travel doors / menu buttons). Server RPC: the
	 *  server snapshots + autosaves (PrepareServerTravel), then asks every
	 *  remote client to prepare (ClientPrepareTravel); once all confirmed, it
	 *  opens the level (ServerTravelNow - clients follow). The barrier makes
	 *  everyone enter the next level from the same moment - no stragglers
	 *  playing in a world the server already left.
	 *
	 *  The MAIN MENU is not another level, it is the way out of the session: a
	 *  guest asking for it only takes that machine home (the room keeps
	 *  running), while the host asking for it dissolves the room for everyone -
	 *  see CloseOnlineSession, which is what the menu button should call. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Travel")
	void RequestTravel(const FString& LevelName);

	/** Reports this machine's player ID once possessed (JoinSession carries no
	 *  name parameter; the host runs the same call locally). The string lands on
	 *  the player state twice over: as the display name, and - when it passes
	 *  the profile whitelist AND no one else in this room is already using it -
	 *  as the ProfileID the host files this player's character under. Without it
	 *  that character is not carried across levels and never reaches a save file
	 *  (see USnGameInstance::GetSessionKey); the refused player is told on the
	 *  HUD, because the refusal is a server-side decision they cannot see. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Travel")
	void ServerSetPlayerName(const FString& PlayerName);

	/** Asks the server to start the pending travel. The barrier state lives on
	 *  the GameInstance, not here: this executes on the CONFIRMING client's own
	 *  controller, which never saw the request - per-controller state would make
	 *  the handshake impossible to complete. */
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "Travel")
	void ServerTravelReady();

	/** Client-side confirmation of the travel barrier: local teardown (close any
	 *  window so the next world starts clean), then confirm. Nothing is written
	 *  here: the travel that follows rebuilds this machine's own player state
	 *  too, but the character it comes back as is the HOST's, captured before
	 *  the travel started - a guest's machine never writes a save file. The one
	 *  exception is a guest leaving the room (its character is captured by the
	 *  host's Logout hook as its connection drops), which needs no local write
	 *  either.
	 *
	 *  With the main menu as the target the guest is not following the server
	 *  into another level - it is leaving the room: its session is torn down
	 *  and it travels itself home. The confirmation still goes out, and that is
	 *  what keeps a dissolve from stranding anyone: the host opens the menu
	 *  level (whose travel drops every remaining connection) only once each
	 *  guest has acknowledged. */
	UFUNCTION(Client, Reliable, Category = "Travel")
	void ClientPrepareTravel(const FString& LevelName);

	/** Closes the online session on THIS machine and returns it to the main
	 *  menu - the entry point behind "close the online room", role-aware:
	 *
	 *   - Guest: only this machine leaves. Its session record is destroyed and
	 *     it travels home; the host and the other guests are untouched (the
	 *     host just sees a normal disconnect).
	 *   - Host: the room ends for everyone. Each guest is told to leave (its own
	 *     session torn down, it travels itself home), the session is destroyed -
	 *     the LAN advertisement disappears from every search - and the host
	 *     follows. bIsHosting clears with the session, so the menu level does
	 *     not come up as a listen server behind a room nobody is in.
	 *
	 *  Local by nature, not an RPC - the session lives on each machine, so
	 *  everyone who wants out runs this for themselves. Call it on the way to
	 *  the main menu: the menu is never inside a session, and a room left open
	 *  keeps showing up in LAN searches with nobody behind it. No-op when this
	 *  machine is not in a session (plain "return to main menu" is
	 *  RequestTravel). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Lobby")
	void CloseOnlineSession();

	/** "+XP" float text for the absorber's own view: absorption runs on the
	 *  server, but the number is local screen-space feedback - delivered here
	 *  instead of painting on the server's HUD (which only the listen host
	 *  would see). */
	UFUNCTION(Client, Reliable, Category = "Loot")
	void ClientShowXPNumber(const FVector& WorldLocation, float Amount);

	/** Unstuck: asks the server to move this player back to the level's spawn
	 *  point (the PlayerStart RestartPlayer uses). The escape hatch for a
	 *  capsule embedded in geometry or a fall out of the world - the menu's
	 *  "脱离卡死" button and the console command Unstuck both call this.
	 *
	 *  Forwarded straight to a Server RPC: the teleport is a write to the pawn's
	 *  transform, which only the server owns (a guest that moved itself locally
	 *  would just be corrected back). On a listen host the RPC body runs inline,
	 *  so one call covers every net mode - the same split the cheat RPCs use.
	 *
	 *  Only THIS player is moved, and only within the current level: no travel,
	 *  no effect on teammates, nothing about the run's state. The team-wipe
	 *  return in ASnGameModeBase is a different path entirely. */
	UFUNCTION(BlueprintCallable, Category = "Recovery")
	void RequestUnstuck();

	// -------------------------------------------------------------------------
	// Chat. Enter opens a one-line input at the bottom of the screen; the text
	// goes to the server, which validates it and relays it from the SPEAKING
	// pawn to every machine, where each one shows a bubble over that pawn's
	// head (see ASnCharacter::MulticastShowChatBubble). No history, no
	// channels, no team split - the bubble is the whole feature.
	//
	// Typing locks the game input (UI-only, cursor stays hidden - see
	// USnWindowManager::ApplyInputModeNow): the player cannot move mid-line,
	// and a click cannot punch through to an attack. That is also why the line
	// is not a window - no cursor, no pause, no Z slot in the manager.
	// -------------------------------------------------------------------------

	/** Opens the chat input line (Enter). No-op while a window is open - in
	 *  UI-only mode the game's Enter binding is dead anyway, so this is a
	 *  guard for the callers that are not the key (the console command). */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void OpenChatInput();

	/** Closes the chat input line: hides the widget and restores the game input
	 *  mode one frame later (the Enter/Escape that closed it is still bubbling
	 *  through Slate - without the deferral the same key would reach the game's
	 *  binding and open the line right back up). Cancel and send both land
	 *  here, so there is exactly one close path. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void CloseChatInput();

	/** Sends a line and closes the input (the widget's Enter handler, and the
	 *  console command's entry point). Trimmed here; an empty line is a cancel
	 *  and never reaches the server. The message is NOT shown locally - the
	 *  bubble every machine shows comes back over the multicast, sender
	 *  included, so what the sender sees is what the server accepted. */
	UFUNCTION(BlueprintCallable, Category = "Chat")
	void SubmitChatText(const FString& Text);

	/** Whether the chat input line is open (the window manager's input mode
	 *  reads this). Local client state, never replicated. */
	UFUNCTION(BlueprintPure, Category = "Chat")
	bool IsChatInputActive() const { return bChatInputActive; }

	/** The Slate widget the input mode focuses while the line is open - the
	 *  text box itself; focusing the outer user widget would leave typing
	 *  unhandled. Null while the widget class is unset. */
	TSharedPtr<SWidget> GetChatFocusWidget() const;

	// -------------------------------------------------------------------------
	// Window manager. Every window (the attribute panel included) is a resident
	// USnActivatableWindow owned by USnWindowManager (lazy-created, see
	// GetWindowManager): instances are Collapsed when closed and re-opened by
	// re-injecting the data source (idempotent SetWidgetController). Several
	// windows can be visible at once (inventory + attribute panel + ...);
	// clicking a window raises it, Escape closes the topmost. Input mode is
	// UI-only while any window is open and game-only when the last one closes -
	// switched by the manager, no manual SetInputMode elsewhere.
	// -------------------------------------------------------------------------

	/** Opens the window and injects its data source. Windows are resident:
	 *  re-opening the same class reuses the instance and re-injects the source
	 *  (idempotent - bindings refresh, never duplicate). Opening an
	 *  already-visible window raises it to the top. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource);

	/** Closes the topmost visible window. Returns false when nothing was open
	 *  (callers should not consume the key/event). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool CloseWindow();

	/** Travel teardown: closes every open window and applies the resulting
	 *  input mode in THIS frame. Called before every world switch (the
	 *  GameInstance does it for every server-side travel, ClientPrepareTravel /
	 *  LeaveSessionLocally for the local ones) and again on arrival in
	 *  BeginPlay. Closes through this controller's CloseWindow so the
	 *  single-player pause pairing (UpdateMenuPause / UpdateDialoguePause)
	 *  converges with it.
	 *
	 *  Applying the mode here is the load-bearing half. Opening a window put the
	 *  game viewport client into UI-only: FInputModeUIOnly releases the mouse
	 *  capture and moves Slate focus off the SViewport (PlayerController.cpp
	 *  ApplyInputMode). The widgets themselves do NOT outlive the level (UMG's
	 *  UGameViewportSubsystem removes them on world cleanup), but that viewport
	 *  input state does - and the next world's controller starts with an EMPTY
	 *  window manager, so nothing there can tell that a window was ever open.
	 *  Only FInputModeGameOnly takes the capture and the focus back, so without
	 *  this call WASD and mouse-look stay dead for the whole level: the
	 *  "main menu -> Continue Game -> cannot move" bug.
	 *
	 *  The manager's usual next-tick deferral cannot cover the travel case:
	 *  UEngine::TickWorldTravel runs before the world ticks, so the level switch
	 *  happens first and the timer goes away with the world. */
	void CloseAllWindowsAndApplyInputMode();

	/** Opens the window if it is closed, closes it (only it - other windows
	 *  stay up) if it is open. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource);

	/** Whether an instance of the given window class is currently visible. */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsWindowOpen(TSubclassOf<USnActivatableWindow> WidgetClass) const;

	/** Opens/closes the attribute panel: a regular window like any other - it
	 *  coexists with the inventory instead of covering it. The resident combat
	 *  widget controller is the data source. Class from AttributePanelWidgetClass. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleAttributePanel();

	/** Opens/closes the developer cheat panel (console command ItemPanel):
	 *  a regular window like any other. Lists every item row; clicking one
	 *  adds it through the inventory's server-authoritative AddItem. Class
	 *  from CheatPanelWidgetClass; the PC itself is the data source (the
	 *  panel has none - the source exists so the first open fires
	 *  WidgetControllerSet and the widget builds its list there). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleCheatPanel();

	/** Opens/closes the in-game main menu (the Esc fallback, see
	 *  HandleEscape): Continue / Settings / Save / Return to main menu / Quit.
	 *  A resident window like any other - the PC is the data source (the menu
	 *  has no controller; the source exists so the first open fires
	 *  WidgetControllerSet, same as the cheat panel). Single player pauses
	 *  behind the menu; LAN never pauses (SetGamePaused freezes every sim -
	 *  see the window-system notes). Pause is paired with menu visibility in
	 *  UpdateMenuPause. */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ToggleMainMenu();

	/** Opens the video settings window (SettingsWidgetClass) above the main
	 *  menu - its button calls this. Stacked like any other window: Escape on
	 *  it closes back to the menu (the topmost). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void OpenSettingsWindow();

	/** Starts a dialogue at FirstRow (the first row's name is the script ID -
	 *  the intro tutorial triggers with "Tutorial.HubIntro"). Looks the row up
	 *  in the global dialogue table and opens the dialogue window
	 *  (DialogueWidgetClass) on a fresh transient controller. Clicking
	 *  advances line by line; Escape skips (the dialogue simply closes). A
	 *  dialogue already on screen restarts from the new row. No-op when the
	 *  first row is missing or the class is unset.
	 *
	 *  bPauseGame: tutorial-style dialogues pause the world behind the popup
	 *  (single player only - the LAN discipline is absolute). The pause is
	 *  paired with the popup's visibility in UpdateDialoguePause, so every
	 *  close path - last line, Escape, travel teardown - releases it. Leave
	 *  false for ambient dialogue (the popup never pauses by itself). */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void PlayDialogue(FName FirstRow, bool bPauseGame = false);

	/** Whether the attribute panel window is currently visible. */
	UFUNCTION(BlueprintPure, Category = "UI")
	bool IsAttributePanelOpen() const;

	/** Opens/closes the inventory window (Blueprint-callable so UI buttons/keys
	 *  can toggle it directly). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ToggleInventory();

	/** Opens the inventory window browsing the given container (chest loot). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenStorageWindow(USnStorageComponent* Storage);

	/** Creates a fresh transient inventory widget controller (the bag + wallet
	 *  data source for window widgets), initialized against this PC. Exposed so
	 *  an interaction action can hand it to OpenConfigWindow as the data source;
	 *  OpenCraftingWindow uses it internally.
	 *
	 *  Deliberately NOT BlueprintPure: every call makes a new instance.
	 *  Controller creation stays on the PC - USnWidgetController::Initialize is
	 *  not BlueprintCallable, so a blueprint-constructed one would never bind. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	USnInventoryWidgetController* CreateInventoryWidgetController();

	/** Opens the crafting window (interaction-triggered): creates a fresh
	 *  transient inventory widget controller and routes through OpenWindow.
	 *  The widget class comes from the interaction row (ConfigWidgetClass). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void OpenCraftingWindow(TSubclassOf<UUserWidget> WidgetClass);

	// -- Legacy entry points, kept as thin forwards so existing BP (interaction
	// -- actions, window buttons) needs no rework. Config windows are windows:
	// -- they stack like any other. --

	/** Opens a config window (interaction-triggered). Forward to the window
	 *  manager. Never pauses the game (SetGamePaused misbehaves in networked
	 *  sessions); UI-only input is the whole story. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OpenConfigWindow(TSubclassOf<UUserWidget> WidgetClass, UObject* DataSource);

	/** Closes the topmost window (forward to CloseWindow). Returns whether a
	 *  window was actually open (false = nothing closed - callers should not
	 *  consume the key/event). */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	bool CloseConfigWindow();

	/** Whether any window is currently open. */
	UFUNCTION(BlueprintPure, Category = "Interaction")
	bool IsConfigWindowOpen() const;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;

	/** Client possession: OnPossess never runs there (that is the server-side
	 *  possess flow), the pawn arrives by replication and OnRep_Pawn fires
	 *  instead. Without a hook here the client would get no input mappings and
	 *  no interaction subscription - WASD/mouse dead, F-key dead. */
	virtual void OnRep_Pawn() override;

	/** The controller's own player state arrived (client only - the server sets
	 *  it directly in PostInitializeComponents and never runs the rep notify).
	 *  One of the readiness events: re-runs TryInitializeUI, because this state
	 *  replicates on its own channel and can land after everything else has
	 *  already tried (the joining client used to get no HUD at all because of
	 *  exactly that). */
	virtual void OnRep_PlayerState() override;

	/** Adds the input mappings and caches/subscribes the pawn's interaction
	 *  component. Runs on both possession paths (server OnPossess, client
	 *  OnRep_Pawn) and re-runs cleanly on respawn. */
	void SetupLocalPlayerPawn(APawn* InPawn);

	/** Input action handlers */
	void HandleMove(const FInputActionValue& Value);
	void HandleLook(const FInputActionValue& Value);
	void HandleJumpStarted(const FInputActionValue& Value);
	void HandleJumpCompleted(const FInputActionValue& Value);
	void HandleToggleAttributePanel(const FInputActionValue& Value);
	void HandleMouseClick(const FInputActionValue& Value);
	void HandleToggleInventory(const FInputActionValue& Value);
	void HandleEscape(const FInputActionValue& Value);
	void HandleToggleCheatPanel(const FInputActionValue& Value);

	/** Selects hotbar slot SlotIndex (numeric keys; bound once per slot with
	 *  the index as the bound parameter). */
	void HandleSelectHotbarSlot(const FInputActionValue& Value, int32 SlotIndex);

	/** Uses the SELECTED hotbar item (right click). */
	void HandleUseHotbarItem(const FInputActionValue& Value);

	/** Enter: opens the chat input line (see OpenChatInput). */
	void HandleChatPressed(const FInputActionValue& Value);

protected:
	/** Mapping context added to the Enhanced Input local player subsystem on possess. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> JumpAction;

	/** Opens/closes the attribute panel (e.g. C key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> AttributePanelAction;

	/** Left click: attacks (windows consume clicks while open - Menu mode). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseClickAction;

	/** Interact with the selected interactable (e.g. F key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InteractAction;

	/** Mouse wheel: cycle the interaction selection. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MouseWheelAction;

	/** Opens/closes the inventory window (e.g. I key). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> InventoryAction;

	/** Fallback Esc handling (e.g. Esc). While a window is open, UI-only mode
	 *  blocks game input - Esc is consumed by the focused window's
	 *  NativeOnKeyDown instead; this binding only catches the no-window case. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> EscapeAction;

	/** Developer cheat panel (ItemPanel - every item row as a button, click to
	 *  add one; the console command route). Debug tooling: leave the BP
	 *  default EMPTY to ship without it - an unset action is never bound. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CheatPanelAction;

	/** Hotbar slot keys (numeric 1..8) in slot order - entry i selects slot i.
	 *  The mapping context blueprints must map them to the actual keys. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TArray<TObjectPtr<UInputAction>> HotbarSlotActions;

	/** Right click: uses the selected hotbar item (consumable / one-use
	 *  ability). Respects the per-item use cooldown. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> UseItemAction;

	/** Opens the chat input line (e.g. Enter). Unset = no chat: the action is
	 *  never bound, and the console command stays the only way to send. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ChatInputAction;

	/** Controller class used for the combat widget controller (falls back to the C++ class). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnCombatWidgetController> CombatWidgetControllerClass;

	/** Controller class used for the ability widget controller (falls back to the C++ class). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnAbilityWidgetController> AbilityWidgetControllerClass;

	/** Interaction HUD widget (prompt for one target, list + highlight for many; fallback tier of the three-tier lookup). */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction")
	TSubclassOf<UUserWidget> InteractionPromptWidgetClass;

	/** Inventory window widget (WBP_Inventory), opened with the I key. Must be
	 *  a USnActivatableWindow subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnActivatableWindow> InventoryWidgetClass;

	/** Attribute panel window (WBP_AttributePanel), toggled with the C key: a
	 *  regular window like the inventory - it coexists with other open windows.
	 *  The combat widget controller is the data source. Must be a
	 *  USnActivatableWindow subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnActivatableWindow> AttributePanelWidgetClass;

	/** Developer cheat panel window (WBP_CheatPanel), toggled from the
	 *  console command ItemPanel (USnCheatManager). Lists every item row in
	 *  the item data table; a click adds one via AddItem (server RPC). Must
	 *  be a USnActivatableWindow subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnActivatableWindow> CheatPanelWidgetClass;

	/** In-game main menu window (WBP_MainMenu), opened by Esc with no window
	 *  open (HandleEscape -> ToggleMainMenu). Stacked with the settings window
	 *  like any other windows. Must be a USnActivatableWindow subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnActivatableWindow> MainMenuWidgetClass;

	/** Video settings window (WBP_Settings), opened from the main menu's
	 *  Settings button (OpenSettingsWindow). Must be a USnActivatableWindow
	 *  subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnActivatableWindow> SettingsWidgetClass;

	/** Dialogue popup (WBP_Dialogue), opened by PlayDialogue - the hub
	 *  GameMode's intro tutorial is its first caller. Click advances a line,
	 *  the last line closes it, Escape skips. Must be a USnActivatableWindow
	 *  subclass. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnActivatableWindow> DialogueWidgetClass;

	/** Minimum seconds between ACCEPTED unstuck requests (0 = no limit). Keeps
	 *  the escape hatch from doubling as a free teleport out of a fight: the
	 *  player who is genuinely stuck waits a few seconds, and so does the one
	 *  hopping across the level to skip it. Enforced on the server (the gate
	 *  guards the teleport), measured against world seconds. */
	UPROPERTY(EditDefaultsOnly, Category = "Recovery", meta = (ClampMin = "0.0"))
	float UnstuckCooldown = 5.f;

	/** Longest accepted chat line, in characters. Enforced on the server (the
	 *  client is untrusted input), and the overlay is never used to paper over
	 *  it: every machine - the sender's included - shows the string the server
	 *  accepted, so a cut line is cut the same way on every screen. */
	UPROPERTY(EditDefaultsOnly, Category = "Chat", meta = (ClampMin = "1"))
	int32 ChatMaxLength = 128;

	/** Minimum seconds between ACCEPTED chat lines from one player (0 = no
	 *  limit). Enforced on the server against world seconds, like the unstuck
	 *  cooldown and for the same reason: the request comes from a client, the
	 *  clock has to be the server's. Dropped lines are silent - chat is not
	 *  worth a feedback popup per keystroke. */
	UPROPERTY(EditDefaultsOnly, Category = "Chat", meta = (ClampMin = "0.0"))
	float ChatCooldown = 0.5f;

	UPROPERTY()
	TObjectPtr<USnCombatWidgetController> CombatWidgetController;

	/** Lazy-created on first access (GetAbilityWidgetController). */
	UPROPERTY()
	TObjectPtr<USnAbilityWidgetController> AbilityWidgetController;

	/** The window manager (lazy-created on first access). */
	UPROPERTY()
	TObjectPtr<USnWindowManager> WindowManager;

	/** Interact: re-scan, then execute the selected target's action (with range/sight re-check). */
	void HandleInteract(const FInputActionValue& Value);

	/** Mouse wheel: cycle the interaction selection. */
	void HandleMouseWheel(const FInputActionValue& Value);

	/** Broadcast from the interaction component: rebuild the transient UI controller. */
	UFUNCTION()
	void HandleInteractablesUpdated(TArray<AActor*> Interactables, int32 SelectedIndex);

	/** Three-tier widget class: action override > row override > PC default. */
	TSubclassOf<UUserWidget> ResolveWidgetClass(const FSnInteractionRow* Row) const;

	/** Looks up the interactable's row in the global interaction table. */
	const FSnInteractionRow* FindInteractionRow(const AActor* Target) const;

	void ExecuteInteraction();
	void UpdateInteractionUI(const TArray<AActor*>& Interactables, int32 SelectedIndex);

	/** Asks the HUD to start the delayed interaction UI teardown (anti-flicker). */
	void ClearInteractionUI();

	/** The pawn's interaction component, cached on possess. */
	UPROPERTY()
	TObjectPtr<UPlayerInteractionComponent> InteractionComponent;

private:
	/** The cheat manager forwards its console commands here (GiveItem); it
	 *  needs access to the private RPC. */
	friend class USnCheatManager;

	/** Server-side execution of the GiveItem cheat command (console input
	 *  routes here from USnCheatManager - the CheatManager is a UObject and
	 *  cannot send RPCs itself). Adds Count of ItemRowName to the inventory
	 *  on the player state; feedback goes back through ClientMessage. */
	UFUNCTION(Server, Reliable)
	void ServerGiveItem(FName ItemRowName, int32 Count);

	/** Server-side execution of the GiveGold cheat command. Positive amounts
	 *  grant, negative amounts take away; the wallet clamps at 0 / MaxGold, so
	 *  the printed result is always the value actually reached. */
	UFUNCTION(Server, Reliable)
	void ServerGiveGold(int64 Amount);

	/** Server-side execution of a Server-class interaction action (the client
	 *  only forwards its selected target; the action class is re-resolved from
	 *  the target's row and the action runs with full authority - see
	 *  ESnActionExecution). */
	UFUNCTION(Server, Reliable)
	void ServerExecuteAction(AActor* Target);

	/** Server-side half of RequestUnstuck (see there): teleports the owning pawn
	 *  to the spawn point AGameModeBase::FindPlayerStart picks - the same source
	 *  RestartPlayer spawns from, so unstuck lands exactly where a fresh spawn
	 *  would, BP overrides of the spawn point included. In co-op that search
	 *  round-robins the starts, so two players unstucking at once do not stack
	 *  on one spot. Silently no-ops without a pawn (dead / mid-travel). */
	UFUNCTION(Server, Reliable)
	void ServerUnstuck();

	/** Server-side half of the chat: the client sends the raw line, the server
	 *  is the only validator (trim, ChatMaxLength, ChatCooldown, a pawn to
	 *  speak from) and then relays the ACCEPTED string to every machine from
	 *  the speaker's pawn - ASnCharacter::MulticastShowChatBubble. The sender's
	 *  own bubble comes from that same multicast, so nothing is echoed locally
	 *  and no client-side rendering can disagree with what was accepted. */
	UFUNCTION(Server, Reliable)
	void ServerSendChatMessage(const FString& Message);

	/** World-seconds timestamp of the last accepted chat line (-1 = never, so
	 *  the first line is never gated). Server copy of this controller only -
	 *  same discipline as LastUnstuckTime. */
	float LastChatTime = -1.f;

	/** True while the chat input line is open on THIS machine. Local client
	 *  state (drives the input mode; never replicated) - the same shape as the
	 *  window manager's open count, which is why the manager reads it. */
	bool bChatInputActive = false;

	/** Server-side execution of the PlaceDevice cheat command (see
	 *  USnCheatManager): places a device from the build-device table where this
	 *  player is looking, exactly the way the placing item does - same aim
	 *  geometry and same validity rule, both from USnBuildLibrary - so a device
	 *  can be tested without the item that grants USnGA_Build. The record goes
	 *  through the GameInstance (snapshot + spawn), which is server work, and the
	 *  outcome comes back as a ClientMessage. Not a player-facing path: it
	 *  skips the item, its cooldown and its cost. */
	UFUNCTION(Server, Reliable)
	void ServerPlaceDevice(FName DeviceID);

	/** World-seconds timestamp of the last accepted unstuck (-1 = never, so the
	 *  first request is never gated). Touched by the server's copy of this
	 *  controller only: the pawn it guards is server-owned, and a client-side
	 *  clock would gate nothing. */
	float LastUnstuckTime = -1.f;

	/** Guest-side half of leaving a session (its own exit, and each guest's run
	 *  of the host's dissolve): drop the menu pause and any open window - the
	 *  same teardown as any travel, see CloseAllWindowsAndApplyInputMode -
	 *  then destroy this machine's session record, mark the GameInstance as out
	 *  of session, and travel home absolutely. ClientTravel is a Client RPC:
	 *  invoked on this machine it executes locally (GEngine::SetClientTravel),
	 *  the same call a guest joins a lobby with. */
	void LeaveSessionLocally();

	/** Re-pairs the single-player pause with the main menu's visibility:
	 *  releases the pause exactly when the menu is gone. Called from every
	 *  path that can leave the menu (CloseWindow, ToggleMainMenu, travel
	 *  teardown) - a stale pause is the classic "game stays frozen" bug, so
	 *  the pairing lives in this one place. No-op when nothing is engaged. */
	void UpdateMenuPause();

	/** Same pairing for pause-type dialogues (PlayDialogue bPauseGame): the
	 *  pause lives exactly as long as the popup stays visible. Called from
	 *  CloseWindow right next to UpdateMenuPause - the two windows never
	 *  overlap, but each pairing keeps its own flag so the release is exact. */
	void UpdateDialoguePause();

	/** True while THIS PC paused the game behind the open main menu. Only ever
	 *  set on a standalone server (LAN sessions never pause, see
	 *  ToggleMainMenu); cleared when the menu closes or the travel tears down. */
	bool bMenuPauseEngaged = false;

	/** True while THIS PC paused the game behind an open pause-type dialogue
	 *  (PlayDialogue bPauseGame). Same discipline as the menu pause: only on
	 *  a standalone server; cleared when the popup closes (every close path
	 *  funnels through CloseWindow -> UpdateDialoguePause). */
	bool bDialoguePauseEngaged = false;
};
