// Editor-only helpers for authoring the item data table.
//
// FSnItemRow hides its 装备 / 使用 blocks behind three panel switches
// (bPanelHas*) that are DERIVED from ItemTags, not hand-ticked - see
// FSnItemRow::SyncPanelSwitches. Every row edit re-derives them through
// UDataTable::HandleDataTableChanged, and imports go through
// FTableRowBase::OnPostDataImport.
//
// Rows written before the switches existed carry the default (false) even
// though their tags say otherwise, so a table that has not been touched since
// would show every item as a bare 基础 + 通用 row. Any edit fixes its own row;
// this command fixes the whole table in one go.

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Game/SnSettings.h"
#include "HAL/IConsoleManager.h"

#if WITH_EDITOR
static FAutoConsoleCommand ResyncItemRowsCommand(
	TEXT("Sn.ResyncItemRows"),
	TEXT("Recompute every row's panel switches (bPanelHas*) in the item data table from its ItemTags."),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		const USnSettings* Settings = USnSettings::Get();
		UDataTable* Table = Settings ? Settings->GetItemDataTable() : nullptr;
		if (!Table)
		{
			UE_LOG(LogTemp, Warning, TEXT("Sn.ResyncItemRows: no item data table configured in Sn Settings."));
			return;
		}

		// NAME_None = every row (the same call the paste-sheet import makes).
		Table->HandleDataTableChanged(NAME_None);

		// The switches are real row data, so the result has to reach the asset
		// or the next load silently reverts to the defaults. Row edits mark the
		// package dirty this way in SRowEditor::NotifyPostChange; a console
		// command has to do it itself (nothing else here dirties the package).
		Table->MarkPackageDirty();

		UE_LOG(LogTemp, Log, TEXT("Sn.ResyncItemRows: %s - synced %d row(s). Save the asset to keep the result."),
			*Table->GetName(), Table->GetRowNames().Num());
	}));
#endif // WITH_EDITOR
