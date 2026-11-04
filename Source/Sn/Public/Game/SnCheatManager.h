// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "SnCheatManager.generated.h"

/**
 * Developer/debug console commands (console key `~`, editor and Development
 * builds). Commands run LOCALLY on the machine that typed them, so anything
 * that mutates server-authoritative state (the inventory lives on the
 * PlayerState) forwards through the PlayerController's server RPC - the
 * CheatManager is a plain UObject and cannot send RPCs itself (community
 * standard split: Exec entry point here, Server RPC on the PC). Standalone
 * executes the RPC inline, so one path works in every net mode.
 */
UCLASS()
class SN_API USnCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	/** GiveItem <ItemID> [Count=1] - adds items to the owning player's
	 *  inventory (server-authoritative via PC's ServerGiveItem RPC).
	 *  Unknown IDs print a hint; ListItems shows every available ID. */
	UFUNCTION(Exec)
	void GiveItem(const FString& ItemID, int32 Count = 1);

	/** GiveGold <Amount> - adds gold to the owning player's wallet
	 *  (server-authoritative via PC's ServerGiveGold RPC). Negative values
	 *  subtract; the result is clamped to 0 .. MaxGold and printed. */
	UFUNCTION(Exec)
	void GiveGold(int64 Amount);

	/** SellItem <ItemID> [Count=1] - sells from the owning player's bag at the
	 *  row's SellPrice. Goes through the same USnInventoryComponent::SellItem
	 *  RPC the shop window will use, so it is an end-to-end wallet check with
	 *  no UI in the loop. Unknown or price-0 IDs print a hint. */
	UFUNCTION(Exec)
	void SellItem(const FString& ItemID, int32 Count = 1);

	/** ListItems - prints every item row name + display name to the console
	 *  (the ID list GiveItem accepts). */
	UFUNCTION(Exec)
	void ListItems();

	/** PlaceDevice <DeviceID> - places a device from the build-device table
	 *  (DT_BuildDevice) where the player is looking, without needing the item
	 *  that normally grants USnGA_Build. Server-authoritative via the PC's
	 *  ServerPlaceDevice RPC; the placement rules are the item path's own, so
	 *  what lands here is what a kit would have placed. Unknown IDs print the
	 *  table's row names. */
	UFUNCTION(Exec)
	void PlaceDevice(const FString& DeviceID);

	/** BuildPreview - dumps the placement-ghost state (local only): the preview
	 *  component and its tick, the selected hotbar row, every USnGA_Build spec
	 *  this ASC has been granted, and the ghost actor on this machine. Reads it
	 *  all from the outside, so it stays a pure readout. */
	UFUNCTION(Exec)
	void BuildPreview();

	/** ItemPanel - opens/closes the developer cheat panel window (every item
	 *  row as a button; click to add one). The window is local client UI;
	 *  adding an item goes through the inventory's AddItem server RPC. */
	UFUNCTION(Exec)
	void ItemPanel();

	/** Unstuck - moves the owning player back to the level's spawn point: the
	 *  console route of the same recovery the menu's "脱离卡死" button runs
	 *  (server-authoritative through the PC, so it works from any net mode).
	 *  Prints the reason when there is nothing to move. */
	UFUNCTION(Exec)
	void Unstuck();

	/** ChatLine <Text> - sends a chat line through the same path the input
	 *  widget uses (SubmitChatText -> ServerSendChatMessage -> the speaker
	 *  pawn's multicast), so the whole relay can be checked in a LAN session
	 *  without opening the input box. Not named "Say": the engine already has
	 *  a console command by that name (UEngine::HandleSayCommand). */
	UFUNCTION(Exec)
	void ChatLine(const FString& Text);
};
