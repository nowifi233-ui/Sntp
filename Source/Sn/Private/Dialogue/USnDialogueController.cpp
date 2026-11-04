// Fill out your copyright notice in the Description page of Project Settings.


#include "Dialogue/USnDialogueController.h"

#include "Dialogue/FSnDialogueRow.h"
#include "Engine/Texture.h" // complete UTexture2D for TSoftObjectPtr::LoadSynchronous
#include "Game/SnSettings.h"

void USnDialogueController::Start(FName FirstRow)
{
	if (FirstRow.IsNone())
	{
		CurrentRow = nullptr;
		return;
	}
	ResolveRow(FirstRow);
}

bool USnDialogueController::Advance()
{
	// Not started or already ended: nothing left to show.
	if (!CurrentRow)
	{
		return false;
	}

	if (CurrentRow->NextRow.IsNone())
	{
		// The last line was just shown - the dialogue is over.
		CurrentRow = nullptr;
		return false;
	}

	ResolveRow(CurrentRow->NextRow);
	return CurrentRow != nullptr;
}

UTexture2D* USnDialogueController::GetPortrait() const
{
	return CurrentRow ? CurrentRow->Portrait.LoadSynchronous() : nullptr;
}

void USnDialogueController::ResolveRow(FName RowName)
{
	UDataTable* Table = USnSettings::Get()->GetDialogueDataTable();
	if (!Table)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnDialogueController::ResolveRow: Sn Settings 未配置 DialogueDataTable"));
		CurrentRow = nullptr;
		return;
	}
	CurrentRow = Table->FindRow<FSnDialogueRow>(RowName, TEXT("USnDialogueController::ResolveRow"));
	if (!CurrentRow)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnDialogueController::ResolveRow: 找不到对话行 %s（链断裂，对话结束）"), *RowName.ToString());
	}
}
