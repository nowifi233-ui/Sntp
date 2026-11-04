// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Inventory/FSnItemTypes.h"
#include "SnSaveTypes.generated.h"

/** Save format version. Bump on any breaking change to the save structs;
 *  saves with an older version refuse to load with a message (no migration).
 *  v2: snapshot stores BASE attributes, equip GE/GA re-applied on restore.
 *  v3: Equipment no longer serializes runtime handles (FSnEquippedItemSave).
 *  v4: Equipment grid replaces fixed slots: sparse FSnGridItem cells (row
 *      name + cell index + rotation), effects derived per layout.
 *  v5: the snapshot splits in two - FSnCharacterSave (what follows the
 *      player) and FSnRunSave (the room's own progress) - so a guest can
 *      save their character without writing someone else's run.
 *  v6: the host keeps the whole room. One file holds EVERY player's
 *      character keyed by player ID (USnSaveGame::Characters) instead of a
 *      single one, and a guest's machine no longer writes its own file.
 *      A file is a ROOM, named after its save slot ("Save_<Slot>.sav") and
 *      not after a player: the slot says WHICH room, the player ID says
 *      WHOSE character. The two are independent, and that needed no bump -
 *      a v6 file was named after the host's ID, which is exactly the ID its
 *      entries are keyed by, so it still opens as the slot of that name.) */
inline constexpr int32 SN_SAVE_VERSION = 6;

/** One player-built device: which device (row in the build-device table) and
 *  where it stands. The record itself is the source of truth and spawned
 *  actors are ephemeral per load - the level that owns the record replays it
 *  on BeginPlay (USnGameInstance::SpawnBuiltDevices). */
USTRUCT(BlueprintType)
struct FSnBuiltDevice
{
	GENERATED_BODY()

	/** Row name (ID) in the build-device data table (USnSettings). */
	UPROPERTY(BlueprintReadWrite, Category = "Building")
	FName DeviceID;

	UPROPERTY(BlueprintReadWrite, Category = "Building")
	FVector Location = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Building")
	FRotator Rotation = FRotator::ZeroRotator;

	/** The level the device was placed IN, normalized exactly like
	 *  FSnRunSave::LevelName ("UEDPIE_0_HubMap" -> "HubMap"), so a record only
	 *  ever replays in the level that owns it - devices are placed in any zone,
	 *  and the furniture of one level must not appear on the floor of the next.
	 *  Empty in a record that was never placed through the build path, which
	 *  then replays everywhere (the pre-tagging behavior). */
	UPROPERTY(BlueprintReadWrite, Category = "Building")
	FString LevelName;
};

/** One row in the global build-device data table (USnSettings). The row name
 *  is the device ID, e.g. "Turret". */
USTRUCT(BlueprintType)
struct FSnBuildDeviceRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Actor class spawned on placement and on save replay (e.g. BP_Turret).
	 *  Hard reference: the data table cell editor cannot pick soft class
	 *  references, and the classes are small (same rule as interaction actions). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Device")
	TSubclassOf<AActor> DeviceClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Device")
	FText DisplayName;

	/** Mesh the client-side placement ghost renders (USnBuildPreviewComponent),
	 *  normally the device blueprint's own body. Per-row because the ghost is
	 *  drawn from data alone - it never instantiates the device class. Empty =
	 *  this device shows no ghost (placement still works). Hard reference for
	 *  the same reason as DeviceClass: the table cell editor cannot pick soft
	 *  object references. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Device")
	TObjectPtr<UStaticMesh> PreviewMesh;
};

/** One save slot's summary for the management list (BP-friendly). */
USTRUCT(BlueprintType)
struct FSnSaveSlotInfo
{
	GENERATED_BODY()

	/** The name of the room: the file is "Save_<SaveSlotID>.sav". A slot is
	 *  deliberately NOT a player ID - the slot says which room, the ID says
	 *  whose character is inside it (see FSnRunSave::HostProfileID). */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString SaveSlotID;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	bool bOccupied = false;

	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FDateTime SaveTime;

	/** Player ID of whoever opened this room (FSnRunSave::HostProfileID).
	 *  Empty in a file written before the field existed. */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString HostProfileID;

	/** How many characters the room holds - one per player ID that has ever
	 *  played in it, the host's own included. */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 CharacterCount = 0;

	/** The HOST's character, for the summary line: the file holds the whole
	 *  party's, and the list describes a room by the person who runs it. */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int32 PlayerLevel = 1;

	/** Wallet balance at capture time (an empty purse reads as 0). */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	int64 Gold = 0;

	/** Map the save continues into (e.g. "HubMap"). */
	UPROPERTY(BlueprintReadOnly, Category = "Save")
	FString LevelName;
};

/**
 * One player's character: everything that travels WITH the person - bag,
 * equipment grid, wallet, base attributes, hotbar bindings.
 *
 * "One profile = one character": this is the part that follows the player.
 * The HOST keeps it - one entry per player ID in its file's table
 * (USnSaveGame::Characters), including the guests', so a night spent in
 * someone else's room leaves the character that was built there, on that
 * machine, under that player's ID. A guest's own machine writes nothing.
 * The room's progress is NOT in here (see FSnRunSave) - a guest must never
 * carry the host's level name, base or position home.
 *
 * Pure logical state, same rule as FSnGridItem: no runtime handles and no
 * spawned actors - the equip GE/GA are derived from the layout on restore
 * (RecomputeGridEffects).
 */
USTRUCT(BlueprintType)
struct FSnCharacterSave
{
	GENERATED_BODY()

	/** The profile this character belongs to (USnGameInstance's player ID).
	 *  Also the key this struct sits under in a save file's table and in the
	 *  session roster, so the two can never disagree about whose character this
	 *  is - the entry the authority hands back to a player's state after a
	 *  travel is the one filed under their ID. */
	UPROPERTY(BlueprintReadWrite, Category = "Character")
	FString ProfileID;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	TArray<FSnItemStack> Inventory;

	/** Equipment grid cells (sparse: only occupied cells). Pure logical
	 *  records - row name + cell index + rotation; the equip GE/GA are derived
	 *  per layout on restore (RecomputeGridEffects). The grid side length is
	 *  not part of the format (indices are plain cell numbers), so saves
	 *  survive a grid resize; out-of-range cells are dropped on restore. */
	UPROPERTY(BlueprintReadWrite, Category = "Character")
	TArray<FSnGridItem> EquipmentGrid;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int64 Gold = 0;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	float XP = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	int32 Level = 1;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	float Health = 100.f;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	float MaxHealth = 100.f;

	/** Absorbing pool in front of Health. Shield is granted by Instant effects
	 *  only (see USnAttributeSet::Shield), so the whole pool lives in the base
	 *  value and round-trips on its own - nothing to re-apply on restore.
	 *  Absent from a file written before the field existed, where it reads as
	 *  the default of 0 (an empty shield is also the correct starting state,
	 *  so that needs no version bump). */
	UPROPERTY(BlueprintReadWrite, Category = "Character")
	float Shield = 0.f;

	UPROPERTY(BlueprintReadWrite, Category = "Character")
	float AttackPower = 10.f;

	/** Inventory slot count. Saved alongside the slots themselves: it may have
	 *  been raised by expansion items, and the saved array can exceed the
	 *  default capacity - restoring one without the other truncates items. */
	UPROPERTY(BlueprintReadWrite, Category = "Character")
	float Capacity = 30.f;

	/** Hotbar slot bindings (row names, in slot order). Not selection - that is
	 *  a session choice and intentionally not saved. An EMPTY array means
	 *  "nothing captured": the restore skips the hotbar and the live bindings
	 *  stay (a default-constructed character must not wipe them) - the guard
	 *  lives in ASnPlayerState::RestoreFromCharacter. */
	UPROPERTY(BlueprintReadWrite, Category = "Character")
	TArray<FName> HotbarSlots;
};

/**
 * The room's own progress: where the run stands and what its players built.
 *
 * This belongs to whoever hosts - it is written to the host's file and never
 * to a guest's. A guest's file may well hold a run of their OWN (from the last
 * time they hosted), and their save must not overwrite it with someone else's.
 *
 * Only differences against the level templates are stored - hub scene changes
 * are milestones (levels render themselves from these on load), built devices
 * are placement records. The world itself never serializes (community-standard
 * for procedural/roguelike games).
 */
USTRUCT(BlueprintType)
struct FSnRunSave
{
	GENERATED_BODY()

	/** Name of the map to travel into on continue (hub or run level). */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	FString LevelName;

	/** Player ID of the person who opened this room, stamped when the file is
	 *  written. Only the menu reads it - the summary line names the room's
	 *  owner, and it is whose character the level/wallet on that line belong
	 *  to. The roster's KEYS are what actually identify characters; this is a
	 *  label. Empty in a file written before the field existed. */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	FString HostProfileID;

	/** Current run floor (run levels only; 0 = in the hub). */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	int32 RunFloor = 0;

	/** The run's difficulty tier (row name in USnSettings::DifficultyDataTable)
	 *  at capture time. Restored to the next world's GameState on entry -
	 *  GameStates are per-level and would otherwise reset to "Normal" on every
	 *  travel (see ASnPlayerState::TryRestoreFromSave). NAME_None = nothing
	 *  captured, which the restore skips (the run stays "Normal"). */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	FName Difficulty;

	/** Unlocked progression milestones (e.g. "Floor2Clear"). The hub level
	 *  renders unlocked areas from these - the scene change itself is never
	 *  serialized, only the progress that drives it. */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	TArray<FName> Milestones;

	/** Placed devices, each tagged with the level that owns it
	 *  (FSnBuiltDevice::LevelName); that level replays its own on load. */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	TArray<FSnBuiltDevice> BuiltDevices;

	/** Where the host stood when the run was captured (captured on the local
	 *  player's state - the room is theirs). Restored only on a continue-game
	 *  entry (the GameInstance's bRestoreLocation flag) and only when non-zero
	 *  - level-to-level travel always reuses PlayerStart. Zero = nothing
	 *  captured, which the restore ignores (never teleport to the origin). */
	UPROPERTY(BlueprintReadWrite, Category = "Run")
	FVector SavedLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadWrite, Category = "Run")
	FRotator SavedRotation = FRotator::ZeroRotator;
};
