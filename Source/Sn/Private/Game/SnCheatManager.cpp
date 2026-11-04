// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/SnCheatManager.h"

#include "AbilitySystem/SnGA_Build.h"
#include "AbilitySystemComponent.h"
#include "Engine/DataTable.h"
#include "GameFramework/Pawn.h"
#include "Game/ASnBuildPreview.h"
#include "Game/Save/SnSaveTypes.h"
#include "Game/USnBuildLibrary.h"
#include "Game/USnBuildPreviewComponent.h"
#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnItemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"
#include "Game/SnSettings.h"

namespace
{
	/** Every build-device row name + display name, for the unknown-ID hint below
	 *  (the device table is small - same discovery idea as ListItems). */
	void PrintBuildDeviceIDs(APlayerController* PC)
	{
		const USnSettings* Settings = USnSettings::Get();
		UDataTable* Table = Settings ? Settings->GetBuildDeviceDataTable() : nullptr;
		if (!Table)
		{
			PC->ClientMessage(TEXT("PlaceDevice: DT_BuildDevice 未配置（Sn Settings 的 BuildDeviceDataTable）"));
			return;
		}

		PC->ClientMessage(TEXT("可用设备 ID："));
		for (const TPair<FName, uint8*>& Entry : Table->GetRowMap())
		{
			const FSnBuildDeviceRow* Row = reinterpret_cast<const FSnBuildDeviceRow*>(Entry.Value);
			if (Row)
			{
				PC->ClientMessage(FString::Printf(TEXT("  %s — %s"), *Entry.Key.ToString(), *Row->DisplayName.ToString()));
			}
		}
	}
}

void USnCheatManager::GiveItem(const FString& ItemID, int32 Count)
{
	const FName RowName(*ItemID);
	if (!USnItemLibrary::FindItemRow(RowName))
	{
		// ClientMessage is a PlayerController member (Client RPC; calling it
		// locally on the client prints directly) - the CheatManager has no
		// message channel of its own, so route through the owning controller.
		GetPlayerController()->ClientMessage(FString::Printf(TEXT("GiveItem: 未知物品 ID '%s'（用 ListItems 查看全部物品）"), *ItemID));
		return;
	}

	// Forward to the server: the inventory is server-authoritative. On a
	// Standalone server the RPC executes inline - one path for every mode.
	if (ASnPlayerController* PC = Cast<ASnPlayerController>(GetPlayerController()))
	{
		PC->ServerGiveItem(RowName, FMath::Max(1, Count));
	}
}

void USnCheatManager::GiveGold(int64 Amount)
{
	if (Amount == 0)
	{
		GetPlayerController()->ClientMessage(TEXT("GiveGold: 数量为 0，钱包没有变化"));
		return;
	}

	// Same split as GiveItem: the CheatManager cannot send RPCs, the PC can.
	// Standalone executes the RPC inline, so one path covers every net mode.
	if (ASnPlayerController* PC = Cast<ASnPlayerController>(GetPlayerController()))
	{
		PC->ServerGiveGold(Amount);
	}
}

void USnCheatManager::SellItem(const FString& ItemID, int32 Count)
{
	APlayerController* PC = GetPlayerController();
	const FName RowName(*ItemID);

	if (!USnItemLibrary::FindItemRow(RowName))
	{
		PC->ClientMessage(FString::Printf(TEXT("SellItem: 未知物品 ID '%s'（用 ListItems 查看全部物品）"), *ItemID));
		return;
	}
	if (USnItemLibrary::GetSellPrice(RowName) <= 0)
	{
		PC->ClientMessage(FString::Printf(TEXT("SellItem: '%s' 没有售价（SellPrice = 0 即不可出售）"), *ItemID));
		return;
	}

	ASnPlayerState* SnPlayerState = PC->GetPlayerState<ASnPlayerState>();
	USnInventoryComponent* Inventory = SnPlayerState ? SnPlayerState->GetInventoryComponent() : nullptr;
	if (!Inventory)
	{
		PC->ClientMessage(TEXT("SellItem: 玩家状态或背包组件不可用"));
		return;
	}

	// The component's SellItem is itself a Server RPC owned by the player
	// state, so this exercises the exact path the shop window will use - from
	// any net mode. INDEX_NONE = sell from wherever it sits in the bag.
	Inventory->SellItem(RowName, FMath::Max(1, Count), INDEX_NONE);
}

void USnCheatManager::ListItems()
{
	APlayerController* PC = GetPlayerController();

	const USnSettings* Settings = USnSettings::Get();
	if (!Settings)
	{
		PC->ClientMessage(TEXT("ListItems: Sn Settings 不可用"));
		return;
	}

	UDataTable* Table = Settings->GetItemDataTable();
	if (!Table)
	{
		PC->ClientMessage(TEXT("ListItems: ItemDataTable 未配置（Sn Settings）"));
		return;
	}

	PC->ClientMessage(TEXT("物品列表（GiveItem 可用 ID）："));
	for (const TPair<FName, uint8*>& Entry : Table->GetRowMap())
	{
		const FSnItemRow* Row = reinterpret_cast<const FSnItemRow*>(Entry.Value);
		if (Row)
		{
			PC->ClientMessage(FString::Printf(TEXT("  %s — %s"), *Entry.Key.ToString(), *Row->DisplayName.ToString()));
		}
	}
}

void USnCheatManager::PlaceDevice(const FString& DeviceID)
{
	APlayerController* PC = GetPlayerController();
	const FName RowName(*DeviceID);

	if (!USnBuildLibrary::FindBuildDeviceRow(RowName))
	{
		PC->ClientMessage(FString::Printf(TEXT("PlaceDevice: 未知设备 ID '%s'（DT_BuildDevice 的行名）"), *DeviceID));
		PrintBuildDeviceIDs(PC);
		return;
	}

	// Same split as GiveItem: the CheatManager cannot send RPCs, the PC can - and
	// the placement has to run on the server anyway (it writes the snapshot and
	// spawns the device). Everything after this is reported through ClientMessage.
	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetPlayerController()))
	{
		SnPC->ServerPlaceDevice(RowName);
	}
}

void USnCheatManager::BuildPreview()
{
	// TEMP DIAGNOSTIC (remove once the ghost has been confirmed on screen). It
	// reads everything from the OUTSIDE - the component's API stays clean, and
	// the whole command disappears in one deletion.
	APlayerController* PC = GetPlayerController();
	APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	if (!Pawn)
	{
		PC->ClientMessage(TEXT("BuildPreview: 没有 Pawn"));
		return;
	}

	const USnBuildPreviewComponent* Preview = Pawn->FindComponentByClass<USnBuildPreviewComponent>();
	PC->ClientMessage(FString::Printf(TEXT("BuildPreview: local=%d component=%s tick=%s"),
		Pawn->IsLocallyControlled() ? 1 : 0,
		Preview ? TEXT("YES") : TEXT("NO"),
		Preview && Preview->IsComponentTickEnabled() ? TEXT("ON") : TEXT("OFF")));

	const ASnPlayerState* SnPS = Pawn->GetPlayerState<ASnPlayerState>();
	const USnInventoryComponent* Inventory = SnPS ? SnPS->FindComponentByClass<USnInventoryComponent>() : nullptr;
	if (!Inventory)
	{
		PC->ClientMessage(TEXT("  背包组件在 PlayerState 上找不到"));
		return;
	}

	const int32 Index = Inventory->GetSelectedHotbarIndex();
	const FName RowName = Inventory->GetSelectedHotbarRowName();
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(RowName);
	UClass* AbilityClass = (Row && Row->UseAbility) ? Row->UseAbility.Get() : nullptr;
	PC->ClientMessage(FString::Printf(TEXT("  index=%d row=%s rowFound=%d useAbility=%s item=%d cd=%.2f"),
		Index, *RowName.ToString(), Row ? 1 : 0, *GetNameSafe(AbilityClass),
		Inventory->HasItem(RowName, 1) ? 1 : 0, Inventory->GetLocalUseCooldownRemaining(RowName)));

	// The gate the ghost reads: a granted USnGA_Build spec. Listing what this
	// ASC holds tells "the grant has not arrived yet" apart from "granted, but
	// not the class this row uses" - the two look identical from the outside.
	UAbilitySystemComponent* ASC = SnPS ? SnPS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		PC->ClientMessage(TEXT("  ASC 不可用（PlayerState 未就绪）"));
		return;
	}

	int32 GrantedBuilds = 0;
	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.Ability && Spec.Ability->IsA<USnGA_Build>())
		{
			++GrantedBuilds;
			PC->ClientMessage(FString::Printf(TEXT("  已授予 %s (Lv%d) rowMatch=%d"),
				*GetNameSafe(Spec.Ability), Spec.Level,
				(AbilityClass && Spec.Ability->GetClass() == AbilityClass) ? 1 : 0));
		}
	}
	if (GrantedBuilds == 0)
	{
		PC->ClientMessage(TEXT("  ASC 上没有任何 USnGA_Build 授予（选中态没同步，或该行的 UseAbility 不是建造类）"));
	}

	// The ghost itself - a local actor, so this lists what is actually here.
	TArray<AActor*> Ghosts;
	UGameplayStatics::GetAllActorsOfClass(Pawn->GetWorld(), ASnBuildPreview::StaticClass(), Ghosts);
	if (Ghosts.Num() == 0)
	{
		PC->ClientMessage(TEXT("  本机没有幽灵 Actor（还没出现过可放置的一帧）"));
	}
	for (const AActor* Ghost : Ghosts)
	{
		PC->ClientMessage(FString::Printf(TEXT("  幽灵: %s hidden=%d loc=%s"),
			*GetNameSafe(Ghost), Ghost->IsHidden() ? 1 : 0, *Ghost->GetActorLocation().ToCompactString()));
	}
}

void USnCheatManager::ChatLine(const FString& Text)
{
	// Straight into the widget's entry point: trim, close-the-line and the
	// server RPC all run exactly as they do for a typed line (the close is a
	// no-op when no box is open). Whatever this prints on the other machines'
	// heads is what a typed message would have.
	if (ASnPlayerController* PC = Cast<ASnPlayerController>(GetPlayerController()))
	{
		PC->SubmitChatText(Text);
	}
}

void USnCheatManager::ItemPanel()
{
	if (ASnPlayerController* PC = Cast<ASnPlayerController>(GetPlayerController()))
	{
		PC->ToggleCheatPanel();
	}
}

void USnCheatManager::Unstuck()
{
	// Same split as GiveItem: the CheatManager is a plain UObject and cannot
	// send RPCs - the PC forwards it to the server (inline on a listen host),
	// so one command covers every net mode. The feedback (cooldown, missing
	// PlayerStart) comes back through the PC's ClientMessage.
	if (ASnPlayerController* PC = Cast<ASnPlayerController>(GetPlayerController()))
	{
		PC->RequestUnstuck();
	}
}
