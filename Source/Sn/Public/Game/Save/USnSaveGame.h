// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Game/Save/SnSaveTypes.h"
#include "GameFramework/SaveGame.h"
#include "USnSaveGame.generated.h"

/**
 * The disk layer: the room + every character that played in it + metadata,
 * written via UGameplayStatics::SaveGameToSlot / loaded via LoadGameFromSlot
 * into the standard Saved/SaveGames directory. Never holds logic - the session
 * layer (USnGameInstance) is the single path in and out of disk.
 *
 * One file per profile ("Save_<PlayerID>.sav"), and the host owns all of it:
 * the file holds the run of whoever hosts ON THIS MACHINE plus the character
 * table of everyone who played there, keyed by player ID. A guest's machine
 * writes no file at all - a guest's character lives in the host's file, which
 * is what lets them rejoin that room with their bag, grid and wallet intact.
 */
UCLASS()
class SN_API USnSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** The room this file's owner hosts. */
	UPROPERTY(VisibleAnywhere, Category = "Save")
	FSnRunSave Run;

	/** Every player's character, keyed by player ID - the room's roster. Holds
	 *  the host's own character under the host's ID as well as each guest's
	 *  under theirs, which is why the menu's summary reads its numbers out of
	 *  the entry for the ID the file is named after. */
	UPROPERTY(VisibleAnywhere, Category = "Save")
	TMap<FString, FSnCharacterSave> Characters;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	FDateTime SaveTime;

	UPROPERTY(VisibleAnywhere, Category = "Save")
	int32 SaveVersion = SN_SAVE_VERSION;
};
