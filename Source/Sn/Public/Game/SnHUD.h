// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "UI/Widget/SnUserWidget.h"
#include "SnHUD.generated.h"

class UUserWidget;
class USnUserWidget;
class ASnPlayerController;
class ASnPlayerState;
class USnCursorWidget;
class USnClickRipple;
class USnDamageNumber;
class USnAttackCooldownWidget;
class USnWaveBannerWidget;
class USnInteractionController;
class USnChatInputWidget;
enum class EDamageType : uint8;

/**
 * Creates the main HUD widget and injects the combat widget controller.
 */
UCLASS()
class SN_API ASnHUD : public AHUD
{
	GENERATED_BODY()

public:
	/** Creates the resident overlay widgets (HUD, cooldown ring, wave banner)
	 *  and injects their widget controllers. Aura pattern: the prerequisites are
	 *  PUSHED IN by the caller - ASnPlayerController::TryInitializeUI, which is
	 *  the readiness gate - instead of being pulled from the owning controller
	 *  here. The overlay must never be created before the player state exists:
	 *  the widget controllers cache the ASC / attribute set once and are never
	 *  re-initialized, so a controller built around a null state stays broken.
	 *  Idempotent - the widgets are created exactly once. */
	void InitializeOverlay(ASnPlayerController* SnPC, ASnPlayerState* SnPS);

	/** Plays the software cursor's click effect (cursor ripple). The ripple
	 *  layer (Z100) sits above the window stack, so window blueprints can call
	 *  this freely. */
	void TriggerCursorClickEffect();

	/** Shows the interaction prompt / picker widget with the given transient controller.
	 *  Resident widget: reused when the class matches, otherwise rebuilt. */
	void ShowInteractionWidget(USnInteractionController* InController, TSubclassOf<UUserWidget> WidgetClass);

	/** Hides the interaction widget (Collapsed - no render, no layout, no teardown). */
	void ClearInteractionWidget();

	/** Shows the chat input line (ChatInputWidgetClass, e.g. WBP_ChatInput) and
	 *  hands it back so the controller can focus its text box. Resident: created
	 *  once on the first Enter, Collapsed when closed - never rebuilt, so the
	 *  text box never has to re-focus against a freshly added Slate widget. */
	USnChatInputWidget* ShowChatInput();

	/** Hides the chat input line (Collapsed, text dropped - the instance stays). */
	void HideChatInput();

	/** The resident chat input line, or null before the first Enter opened one.
	 *  The controller focuses its text box through this (owning stays here). */
	USnChatInputWidget* GetChatInputWidget() const { return ChatInputWidget; }

	/** Spawns a screen-space floating damage number at the world location
	 *  (local view), styled by the hit's element (rich-text tag
	 *  "Damage.<Element>"). */
	void ShowDamageNumber(const FVector& WorldLocation, float Amount, EDamageType DamageType);

	/** Crit variant: same element style with a "!" suffix, shares the pool. */
	void ShowCriticalDamageNumber(const FVector& WorldLocation, float Amount, EDamageType DamageType);

	/** Dodge variant: "MISS" text ("Damage.Miss" style), shares the pool. */
	void ShowMissNumber(const FVector& WorldLocation);

	/** Green "+XP" variant ("XP" style), sharing the same widget pool. */
	void ShowXPNumber(const FVector& WorldLocation, float Amount);

	/** Shows the hovered item's detail popup. One resident instance: created
	 *  once on first call, then reused for every hover (no per-hover
	 *  create/destroy flash). The popup is pure BP - slots inject the hovered
	 *  item's data via GetItemDetailWidget, then call this to show it
	 *  (OnMouseEnter); OnMouseLeave calls HideItemDetail. */
	UFUNCTION(BlueprintCallable, Category = "ItemDetail")
	void ShowItemDetail();

	/** The resident detail popup (created once on first ShowItemDetail). Slots
	 *  set the hovered item's data on it directly; the widget itself is pure BP. */
	UFUNCTION(BlueprintPure, Category = "ItemDetail")
	UUserWidget* GetItemDetailWidget() const { return ItemDetailWidget; }

	/** Hides the detail popup (Collapsed - the instance stays alive). */
	UFUNCTION(BlueprintCallable, Category = "ItemDetail")
	void HideItemDetail();

	/** Shows the BOND detail popup - same resident-instance pattern as the
	 *  item detail popup, but controller-driven: every show creates a fresh
	 *  transient USnInventoryWidgetController (same pattern as window opens,
	 *  no stale bindings) and injects it via SetWidgetController, so the
	 *  widget binds its bond queries (GetActiveBonds / GetBondLevel /
	 *  GetBondTierDescriptions... through GetInventoryComponent) in
	 *  WidgetControllerSet and refreshes through the controller's
	 *  OnEquipmentChanged (equip changes = bond counts change). The two
	 *  detail popups are exclusive: showing one hides the other (switch). */
	UFUNCTION(BlueprintCallable, Category = "ItemDetail")
	void ShowBondDetail();

	/** The resident bond detail popup (created once on first ShowBondDetail). */
	UFUNCTION(BlueprintPure, Category = "ItemDetail")
	USnUserWidget* GetBondDetailWidget() const { return BondDetailWidget; }

	/** Hides the bond detail popup (Collapsed - the instance stays alive). */
	UFUNCTION(BlueprintCallable, Category = "ItemDetail")
	void HideBondDetail();

	/** Shows the carried item's icon on the software cursor (click-pickup:
	 *  the cursor icon follows the mouse for free - the engine moves the
	 *  software cursor). The item slots drive this via USnInventorySlotWidget. */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void ShowCarriedItem(FName ItemRowName);

	/** Hides the carried item icon (carry session ended). */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void HideCarriedItem();

protected:
	virtual void BeginPlay() override;

	/** Creates the software cursor widget and hands it to the engine (follows the mouse, no Tick). */
	void EnsureCursorWidget();

	/** Creates the click ripple widget (separate top-level layer, not clipped by the cursor). */
	void EnsureRippleWidget();

	/** Main HUD widget spawned in BeginPlay (e.g. WBP_HUD). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnUserWidget> HUDWidgetClass;

	/** Software cursor widget, registered with the viewport client on first use. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnCursorWidget> CursorWidgetClass;

	UPROPERTY()
	TObjectPtr<USnCursorWidget> CursorWidget;

	/** Click ripple widget shown above the cursor while the panel is open. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnClickRipple> RippleWidgetClass;

	UPROPERTY()
	TObjectPtr<USnClickRipple> RippleWidget;

	/** Interaction prompt / picker widget instance, resident and recycled here. */
	UPROPERTY()
	TObjectPtr<UUserWidget> InteractionWidget;

	/** Resident hotbar bar widget, created once in InitializeOverlay. */
	UPROPERTY()
	TObjectPtr<USnUserWidget> HotbarWidget;

	/** Resident chat input line, created once on the first Enter and Collapsed
	 *  while closed. */
	UPROPERTY()
	TObjectPtr<USnChatInputWidget> ChatInputWidget;

	/** Resident detail popup instance, created once on first ShowItemDetail. */
	UPROPERTY()
	TObjectPtr<UUserWidget> ItemDetailWidget;

	/** Resident bond detail popup instance, created once on first
	 *  ShowBondDetail (controller-driven - must be a USnUserWidget). */
	UPROPERTY()
	TObjectPtr<USnUserWidget> BondDetailWidget;

	/** Floating damage number widget (e.g. WBP_DamageNumber). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnDamageNumber> DamageNumberWidgetClass;

	/** Hovered-item detail popup (e.g. WBP_ItemDetail) - pure BP content. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ItemDetailWidgetClass;

	/** Bond detail popup (e.g. WBP_BondDetail) - controller-driven, must be a
	 *  USnUserWidget subclass (SetWidgetController injects the inventory
	 *  controller; the widget binds its bond queries in WidgetControllerSet). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnUserWidget> BondDetailWidgetClass;

	/** Ring attack-cooldown indicator at the screen center (e.g. WBP_AttackCooldown). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnAttackCooldownWidget> AttackCooldownWidgetClass;

	/** Top-center wave banner fed by the survival spawner (e.g. WBP_WaveBanner). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnWaveBannerWidget> WaveBannerWidgetClass;

	/** Bottom hotbar bar (e.g. WBP_Hotbar) showing the replicated hotbar slots
	 *  and the current selection. Data source: the inventory component itself
	 *  (the widget binds its hotbar queries + OnHotbarChanged in BP). */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnUserWidget> HotbarWidgetClass;

	/** Chat input line (e.g. WBP_ChatInput), opened by Enter. Must be a
	 *  USnChatInputWidget subclass - the base owns Enter/Escape/clear, and the
	 *  named text box it binds is what the input mode focuses. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<USnChatInputWidget> ChatInputWidgetClass;

	/** Hard cap on concurrent damage/XP number widgets; beyond it, numbers are dropped. */
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	int32 DamageNumberPoolSize = 16;

	/** Shared pool path for damage / XP / crit / miss numbers; InStyleTag names
	 *  the rich-text style row, InTextOverride replaces the numeric text. */
	void ShowPooledNumber(const FVector& WorldLocation, float Amount, const FName InStyleTag, bool bPrefixPlus, bool bAppendBang, const FString& InTextOverride = TEXT(""));

	/** Finished numbers, recycled by ShowDamageNumber instead of being created and destroyed. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<USnDamageNumber>> DamageNumberPool;

	/** Total widgets ever created (grows to DamageNumberPoolSize, then stops). */
	int32 DamageNumbersCreated = 0;

	/** Monotonic scatter counter fed to the damage number widget: consecutive
	 *  shows land on different golden-angle ring offsets, so numbers from one
	 *  attack (physical + enchant hits) never stack. */
	int32 NextNumberIndex = 0;

	/** True once the overlay widgets exist; InitializeOverlay is a no-op after. */
	bool bOverlayInitialized = false;
};
