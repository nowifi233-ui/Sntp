// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SnHUD.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Combat/USnCombatLibrary.h"
#include "Interaction/USnInteractionController.h"
#include "Inventory/USnInventoryComponent.h"
#include "Spawn/SnSurvivalSpawner.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"
#include "UI/Widget/SnAttackCooldownWidget.h"
#include "UI/Widget/SnChatInputWidget.h"
#include "UI/Widget/SnClickRipple.h"
#include "UI/WidgetController/SnCombatWidgetController.h"
#include "UI/Widget/SnCursorWidget.h"
#include "UI/Widget/SnDamageNumber.h"
#include "UI/Widget/SnUserWidget.h"
#include "UI/Widget/SnWaveBannerWidget.h"
#include "UI/WidgetController/USnInventoryWidgetController.h"

void ASnHUD::BeginPlay()
{
	Super::BeginPlay();

	// Ask the owning controller to run its readiness gate rather than guessing
	// here: on a client this HUD is spawned by ClientSetHUD, which can land
	// before or after the player state's replication. TryInitializeUI calls
	// back into InitializeOverlay the moment both halves exist - now, or from
	// its OnRep_PlayerState later.
	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetOwningPlayerController()))
	{
		SnPC->TryInitializeUI();
	}
}

void ASnHUD::InitializeOverlay(ASnPlayerController* SnPC, ASnPlayerState* SnPS)
{
	if (bOverlayInitialized)
	{
		return;
	}

	// The readiness gate is ASnPlayerController::TryInitializeUI - it only calls
	// this with both halves present. The check stays (rather than an assert) so
	// that a future caller skipping the gate fails loudly instead of silently
	// rebuilding the bug this replaced: widget controllers created around a null
	// player state are cached forever and never re-initialized.
	if (!SnPC || !SnPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnHUD::InitializeOverlay: no controller / player state yet - overlay not created (call ASnPlayerController::TryInitializeUI instead)"));
		return;
	}

	// The attack-cooldown ring sits above everything, always resident, fed by
	// the same combat widget controller as the main HUD. It starts collapsed
	// and only shows while a cooldown runs.
	if (AttackCooldownWidgetClass)
	{
		USnAttackCooldownWidget* CooldownWidget = CreateWidget<USnAttackCooldownWidget>(SnPC, AttackCooldownWidgetClass);
		if (CooldownWidget)
		{
			CooldownWidget->AddToViewport(10);
			CooldownWidget->SetWidgetController(SnPC->GetCombatWidgetController());
		}
	}

	// Wave banner: resident, fed by the survival spawner's OnWaveChanged through
	// the same SetWidgetController injection as every other widget. Only exists
	// when the level actually has a spawner (no spawner = no banner).
	if (WaveBannerWidgetClass)
	{
		ASnSurvivalSpawner* Spawner = nullptr;
		for (TActorIterator<ASnSurvivalSpawner> It(GetWorld()); It; ++It)
		{
			Spawner = *It;
			break;
		}
		{
			USnWaveBannerWidget* Banner = CreateWidget<USnWaveBannerWidget>(SnPC, WaveBannerWidgetClass);
			if (Banner)
			{
				Banner->AddToViewport(10);
				Banner->SetWidgetController(Spawner);
			}
		}
	}

	// Hotbar bar: resident at the bottom of the screen (Z 5 - above the main
	// HUD, below the cooldown ring/banner and the window stack). Its data
	// source is the inventory component itself, not a widget controller: the
	// widget binds its hotbar queries (GetHotbarSlot / GetSelectedHotbarIndex
	// / GetLocalUseCooldownRemaining) in WidgetControllerSet and refreshes
	// through OnHotbarChanged / OnInventoryChanged (both replicated events
	// broadcast on this side too). Exists on every side - the hotbar state
	// replicates with the component.
	if (HotbarWidgetClass)
	{
		if (USnInventoryComponent* Inventory = SnPS->GetInventoryComponent())
		{
			HotbarWidget = CreateWidget<USnUserWidget>(SnPC, HotbarWidgetClass);
			if (HotbarWidget)
			{
				HotbarWidget->AddToViewport(5);
				HotbarWidget->SetWidgetController(Inventory);
			}
		}
	}

	if (!HUDWidgetClass)
	{
		bOverlayInitialized = true;
		return;
	}

	USnUserWidget* HUDWidget = CreateWidget<USnUserWidget>(SnPC, HUDWidgetClass);
	if (!HUDWidget)
	{
		return;
	}

	// Inject the widget controller: created lazily by the player controller,
	// the widget binds to its delegates in WidgetControllerSet.
	HUDWidget->SetWidgetController(SnPC->GetCombatWidgetController());

	HUDWidget->AddToViewport();
	bOverlayInitialized = true;
}

void ASnHUD::EnsureCursorWidget()
{
	if (CursorWidget || !CursorWidgetClass)
	{
		return;
	}

	CursorWidget = CreateWidget<USnCursorWidget>(GetOwningPlayerController(), CursorWidgetClass);
	if (CursorWidget)
	{
		// Hand the widget to the engine: it becomes the software cursor and
		// follows the mouse automatically - no Tick positioning, no lag.
		GEngine->GameViewport->SetSoftwareCursorWidget(EMouseCursor::Default, CursorWidget->TakeWidget());
	}
}

void ASnHUD::TriggerCursorClickEffect()
{
	// Called by window blueprints when they want a click ripple. No guard here:
	// the caller decides when it is meaningful.
	EnsureCursorWidget();
	EnsureRippleWidget();

	if (RippleWidget)
	{
		RippleWidget->PlayAt(UWidgetLayoutLibrary::GetMousePositionOnViewport(this));
	}
}

void ASnHUD::EnsureRippleWidget()
{
	if (RippleWidget || !RippleWidgetClass)
	{
		return;
	}

	RippleWidget = CreateWidget<USnClickRipple>(GetOwningPlayerController(), RippleWidgetClass);
	if (RippleWidget)
	{
		// Above everything else; it must be Hit Test Invisible in the designer.
		RippleWidget->AddToViewport(100);
	}
}

void ASnHUD::ShowInteractionWidget(USnInteractionController* InController, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!InController || !WidgetClass)
	{
		ClearInteractionWidget();
		return;
	}

	// Reuse the resident widget if the class matches, otherwise rebuild it.
	if (!InteractionWidget || InteractionWidget->GetClass() != WidgetClass)
	{
		if (InteractionWidget)
		{
			InteractionWidget->RemoveFromParent();
			InteractionWidget = nullptr;
		}
		InteractionWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(), WidgetClass);
		if (InteractionWidget)
		{
			InteractionWidget->AddToViewport();
		}
	}

	if (!InteractionWidget)
	{
		return;
	}

	// Resident widget: show and hand over the new data. No teardown, no
	// flicker - brief scan gaps just leave it Collapsed.
	InteractionWidget->SetVisibility(ESlateVisibility::Visible);
	if (USnUserWidget* SnWidget = Cast<USnUserWidget>(InteractionWidget))
	{
		SnWidget->SetWidgetController(InController);
	}
}

void ASnHUD::ClearInteractionWidget()
{
	if (InteractionWidget)
	{
		InteractionWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

USnChatInputWidget* ASnHUD::ShowChatInput()
{
	// Lazy like the detail popups: nothing is built until the first Enter, and
	// from then on it is the same instance going Visible / Collapsed (the
	// resident-widget rule - rebuilding per open would focus a text box whose
	// Slate widget was added in the same frame).
	if (!ChatInputWidget && ChatInputWidgetClass)
	{
		ChatInputWidget = CreateWidget<USnChatInputWidget>(GetOwningPlayerController(), ChatInputWidgetClass);
		if (ChatInputWidget)
		{
			ChatInputWidget->AddToViewport();
		}
	}

	if (ChatInputWidget)
	{
		ChatInputWidget->SetVisibility(ESlateVisibility::Visible);
		ChatInputWidget->Activate();
	}

	return ChatInputWidget;
}

void ASnHUD::HideChatInput()
{
	if (ChatInputWidget)
	{
		ChatInputWidget->Deactivate();
		ChatInputWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASnHUD::ShowDamageNumber(const FVector& WorldLocation, float Amount, EDamageType DamageType)
{
	ShowPooledNumber(WorldLocation, Amount, USnCombatLibrary::GetDamageNumberTag(DamageType), /*bPrefixPlus=*/false, /*bAppendBang=*/false);
}

void ASnHUD::ShowCriticalDamageNumber(const FVector& WorldLocation, float Amount, EDamageType DamageType)
{
	// Same element style, "!" suffix - the crit reads as "bigger, exclaiming"
	// instead of a color swap.
	ShowPooledNumber(WorldLocation, Amount, USnCombatLibrary::GetDamageNumberTag(DamageType), /*bPrefixPlus=*/false, /*bAppendBang=*/true);
}

void ASnHUD::ShowMissNumber(const FVector& WorldLocation)
{
	ShowPooledNumber(WorldLocation, 0.f, TEXT("Damage.Miss"), /*bPrefixPlus=*/false, /*bAppendBang=*/false, TEXT("MISS"));
}

void ASnHUD::ShowXPNumber(const FVector& WorldLocation, float Amount)
{
	ShowPooledNumber(WorldLocation, Amount, TEXT("XP"), /*bPrefixPlus=*/true, /*bAppendBang=*/false);
}

void ASnHUD::ShowItemDetail()
{
	if (!ItemDetailWidgetClass)
	{
		return;
	}

	// One resident instance, created once on first call and reused for every
	// hover - no per-hover create/destroy churn under fast mouse sweeps (no
	// flash). Z 80: above the window stack (20+), below the click ripple (100).
	// The popup is pure BP; the calling slot injects the item data via
	// GetItemDetailWidget before showing.
	if (!ItemDetailWidget)
	{
		ItemDetailWidget = CreateWidget<UUserWidget>(GetOwningPlayerController(), ItemDetailWidgetClass);
		if (ItemDetailWidget)
		{
			ItemDetailWidget->AddToViewport(80);
		}
	}

	if (ItemDetailWidget)
	{
		ItemDetailWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// The two detail popups are exclusive - showing one hides the other.
	HideBondDetail();
}

void ASnHUD::HideItemDetail()
{
	if (ItemDetailWidget)
	{
		ItemDetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASnHUD::ShowBondDetail()
{
	if (!BondDetailWidgetClass)
	{
		return;
	}

	// One resident instance, created once on first call and reused - same
	// pattern as ShowItemDetail (no per-hover create/destroy churn), same Z 80
	// (above the window stack 20+, below the click ripple 100).
	if (!BondDetailWidget)
	{
		BondDetailWidget = CreateWidget<USnUserWidget>(GetOwningPlayerController(), BondDetailWidgetClass);
		if (BondDetailWidget)
		{
			BondDetailWidget->AddToViewport(80);
		}
	}
	if (!BondDetailWidget)
	{
		return;
	}

	// Fresh transient controller per show (same as window opens, no stale
	// bindings): injects the inventory data source - the widget binds its bond
	// queries in WidgetControllerSet, BroadcastInitialValues pushes the
	// current state. Equip changes refresh through OnEquipmentChanged.
	USnInventoryWidgetController* Controller = NewObject<USnInventoryWidgetController>(GetOwningPlayerController());
	Controller->Initialize(GetOwningPlayerController());
	BondDetailWidget->SetWidgetController(Controller);

	BondDetailWidget->SetVisibility(ESlateVisibility::Visible);

	// The two detail popups are exclusive - showing one hides the other.
	HideItemDetail();
}

void ASnHUD::HideBondDetail()
{
	if (BondDetailWidget)
	{
		BondDetailWidget->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void ASnHUD::ShowCarriedItem(FName ItemRowName)
{
	EnsureCursorWidget();
	if (CursorWidget)
	{
		CursorWidget->SetCarriedItem(ItemRowName);
	}
}

void ASnHUD::HideCarriedItem()
{
	if (CursorWidget)
	{
		CursorWidget->ClearCarriedItem();
	}
}

void ASnHUD::ShowPooledNumber(const FVector& WorldLocation, float Amount, const FName InStyleTag, bool bPrefixPlus, bool bAppendBang, const FString& InTextOverride)
{
	if (!DamageNumberWidgetClass)
	{
		return;
	}

	// Pool first: finished numbers are reused, so the widget count plateaus at
	// DamageNumberPoolSize - no create/destroy churn, no GC pressure, even
	// under burst damage. Beyond the cap, new numbers are simply dropped.
	USnDamageNumber* Number = DamageNumberPool.Num() > 0 ? DamageNumberPool.Pop() : nullptr;

	if (!Number && DamageNumbersCreated < DamageNumberPoolSize)
	{
		Number = CreateWidget<USnDamageNumber>(GetOwningPlayerController(), DamageNumberWidgetClass);
		if (Number)
		{
			Number->AddToViewport();
			++DamageNumbersCreated;

			// Return to the pool when this widget's float finishes. The lambda
			// captures the HUD weakly so a stale number can never outlive it.
			Number->OnFinished.BindWeakLambda(this, [this, Number]()
			{
				DamageNumberPool.Push(Number);
			});
		}
	}

	if (Number)
	{
		Number->ShowAt(WorldLocation, Amount, InStyleTag, bPrefixPlus, bAppendBang, InTextOverride, NextNumberIndex++);
	}
}
