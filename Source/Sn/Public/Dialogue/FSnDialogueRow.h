// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FSnDialogueRow.generated.h"

class UTexture2D;

/** One dialogue line (row name = the line's ID). Lines chain through NextRow
 *  into a script: the trigger passes the FIRST line's row name, every click
 *  advances to NextRow, and an empty NextRow ends the dialogue. A table row is
 *  pure data - speaker / portrait / text can all be edited in the DataTable
 *  editor with zero C++ (this is also what the intro tutorial is built on:
 *  row chain "Tutorial.HubIntro" -> ..._2 -> ..._3 -> ..._4). */
USTRUCT(BlueprintType)
struct SN_API FSnDialogueRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Speaker name shown above the line. Empty = narration (no name shown). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText SpeakerName;

	/** Character portrait next to the line. Empty = no portrait (hidden). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** The line's text. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText LineText;

	/** Row name of the next line. Empty = the dialogue ends after this line. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName NextRow;
};
