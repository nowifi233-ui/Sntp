// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Window/SnWindowManager.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Player/SnPlayerController.h"
#include "TimerManager.h"
#include "UI/Window/SnActivatableWindow.h"
#include "Widgets/SWidget.h"

void USnWindowManager::Initialize(ASnPlayerController* InPC)
{
	PC = InPC;
}

void USnWindowManager::OpenWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource)
{
	if (!PC || !WidgetClass)
	{
		return;
	}

	// Widgets are per-machine UI: only the LOCAL player controller can own one.
	// A non-local controller here means a window-opening path ran on the server
	// for somebody else's player - the window would land on the HOST's screen
	// while the interacting client sees nothing. The usual cause is an
	// interaction action whose Execution is Server: UI actions must stay Client
	// (only world-modifying actions belong behind ServerExecuteAction), and a
	// dialogue opened from server-side blueprint logic (tutorial milestone,
	// world timer) lands here too. Same predicate as the engine's own widget
	// check - this just fires first and names the widget that was asked for.
	if (!PC->IsLocalPlayerController())
	{
		UE_LOG(LogTemp, Warning, TEXT("OpenWindow: %s 不能在非本地 PC（%s）上打开——开窗路径跑到服务器上了。检查交互动作的 Execution（UI 动作必须是 Client，不是 Server），或从服务器侧蓝图调用的 PlayDialogue。"),
			*GetNameSafe(WidgetClass.Get()), *GetNameSafe(PC));
		return;
	}

	USnActivatableWindow* Window = FindWindow(WidgetClass);

	// A resident instance that is ALREADY on screen is not being opened, only
	// re-targeted (opening a storage chest reuses the inventory widget and swaps
	// its data source). The open hook below must not fire for that - the window
	// would replay its animation while sitting in front of the player. A
	// brand-new instance always counts as an open, whatever the class default
	// visibility is.
	bool bWasVisible = false;
	if (Window)
	{
		bWasVisible = Window->GetVisibility() == ESlateVisibility::Visible;
	}
	else
	{
		Window = CreateWidget<USnActivatableWindow>(PC, WidgetClass);
		if (!Window)
		{
			return;
		}
		Window->AddToViewport(WindowBaseZ + Windows.Num());
		Windows.Add(Window);
	}

	// Order matters: show first (the focus target must be visible), then raise
	// (Z + focus), then inject the data source - idempotent, so a reused
	// instance refreshes instead of re-binding.
	Window->SetVisibility(ESlateVisibility::Visible);
	RaiseWindow(Window);
	Window->SetWidgetController(DataSource);

	// BP hook for the open animation ("Event OnWindowOpened -> Play Animation").
	// Every step above has to be done first: the widget must be VISIBLE (UMG only
	// advances an animation while its widget is visible - UMGSequenceTickManager
	// checks IsVisible() every frame and drops the animation data of a hidden
	// one), it must be raised (a resident instance is re-opened through
	// RemoveFromParent + AddToViewport, which rebuilds the slate node the
	// animation drives), and it must be populated (otherwise an empty window
	// fades in and its contents pop in afterwards). UpdateInputMode stays last,
	// like CloseWindowInstance.
	if (!bWasVisible)
	{
		Window->OnWindowOpened();
	}

	UpdateInputMode();
}

bool USnWindowManager::CloseWindow()
{
	// Topmost first: resident instances below are Collapsed and skipped.
	for (int32 i = Windows.Num() - 1; i >= 0; --i)
	{
		if (USnActivatableWindow* Window = Windows[i])
		{
			if (Window->GetVisibility() == ESlateVisibility::Visible)
			{
				CloseWindowInstance(Window);
				return true;
			}
		}
	}
	return false;
}

void USnWindowManager::CloseWindowInstance(USnActivatableWindow* Window)
{
	if (!Window || Window->GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}

	Window->SetVisibility(ESlateVisibility::Collapsed);

	// BP hook: unpause the spawner menu, reset per-open state, ...
	Window->OnWindowClosed();

	// Keyboard focus moves to the next topmost window; with none left the game
	// takes over (Collapsed widgets drop out of the focus path automatically).
	for (int32 i = Windows.Num() - 1; i >= 0; --i)
	{
		if (USnActivatableWindow* Next = Windows[i])
		{
			if (Next != Window && Next->GetVisibility() == ESlateVisibility::Visible)
			{
				Next->SetUserFocus(PC);
				break;
			}
		}
	}

	UpdateInputMode();
}

void USnWindowManager::ToggleWindow(TSubclassOf<USnActivatableWindow> WidgetClass, UObject* DataSource)
{
	if (!WidgetClass)
	{
		return;
	}

	if (IsWindowOpen(WidgetClass))
	{
		// Close this window only - other open windows (attribute panel next to
		// the inventory) stay up.
		CloseWindowInstance(FindWindow(WidgetClass));
	}
	else
	{
		OpenWindow(WidgetClass, DataSource);
	}
}

bool USnWindowManager::IsWindowOpen(TSubclassOf<USnActivatableWindow> WidgetClass) const
{
	if (const USnActivatableWindow* Window = FindWindow(WidgetClass))
	{
		return Window->GetVisibility() == ESlateVisibility::Visible;
	}
	return false;
}

bool USnWindowManager::IsAnyWindowOpen() const
{
	for (const USnActivatableWindow* Window : Windows)
	{
		if (Window && Window->GetVisibility() == ESlateVisibility::Visible)
		{
			return true;
		}
	}
	return false;
}

void USnWindowManager::RaiseWindow(USnActivatableWindow* Window)
{
	if (!Window || !Windows.Contains(Window))
	{
		return;
	}

	if (Windows.Last() != Window)
	{
		// Re-add at a higher Z (RemoveFromParent + AddToViewport - UMG has no
		// SetZOrderInViewport; the slate node rebuild is fine for the rare
		// raise). Z = Base + index keeps the tail on top.
		Windows.Remove(Window);
		Windows.Add(Window);
		Window->RemoveFromParent();
		Window->AddToViewport(WindowBaseZ + Windows.Num() - 1);
	}

	// (Re)focus even when already topmost - a click can leave focus on a child.
	Window->SetUserFocus(PC);
}

USnActivatableWindow* USnWindowManager::FindWindow(TSubclassOf<USnActivatableWindow> WidgetClass) const
{
	for (USnActivatableWindow* Window : Windows)
	{
		if (Window && Window->GetClass() == WidgetClass)
		{
			return Window;
		}
	}
	return nullptr;
}

void USnWindowManager::UpdateInputMode()
{
	if (!PC)
	{
		return;
	}

	// Windows up (first open or stacked): apply UI-only + cursor IMMEDIATELY.
	// Never defer into a timer here - a pause-type dialogue (PlayDialogue
	// bPauseGame) freezes the world in the SAME frame, and a deferred switch
	// would never run: the cursor would stay hidden behind the popup (the
	// hub-intro tutorial used to hit this: it opens from a world-timer
	// callback, so its next-tick timer was queued after the timer pass and
	// froze with the world). Opening from game input is safe to apply
	// mid-frame - that event is already delivered, nothing re-fires it.
	if (IsAnyWindowOpen())
	{
		ApplyInputModeNow();
		return;
	}

	// Closing the last window: defer to next frame - the key that closed it
	// (B/C from the focused window's BP OnKeyDown) is still bubbling through
	// Slate while this runs. Switching to GameOnly here would flip
	// SetIgnoreInput(false) mid-event, so the same physical press reaches the
	// game's Enhanced Input and re-toggles the window (closed-and-reopened =
	// "press B twice to close"). Next frame the event is long gone.
	PC->GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]() { ApplyInputModeNow(); }));
}

void USnWindowManager::ApplyInputModeNow()
{
	// The chat input line outranks the windows, and is NOT a window: the player
	// keeps playing with the cursor hidden, so it needs a third state rather
	// than a slot in the stack. UI-only with the text box focused (without it
	// the box is visible but swallows nothing - the keys would go to Slate's
	// default focus), and no cursor: the mouse is still aiming the camera, it
	// just cannot act through UI-only input.
	//
	// Windows cannot be open here in practice - they are also UI-only, so the
	// game's Enter action cannot fire while one is up - but the order is kept
	// anyway so a future path that opens both still types rather than losing
	// the keystrokes.
	if (PC && PC->IsChatInputActive())
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeUIOnly()
			.SetWidgetToFocus(PC->GetChatFocusWidget())
			.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
		return;
	}

	// Community-recommended rule: first window open -> UI-only + cursor; last
	// window closed -> game-only, cursor hidden (third-person look works again).
	if (IsAnyWindowOpen())
	{
		PC->bShowMouseCursor = true;
		PC->SetInputMode(FInputModeUIOnly().SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock));
	}
	else
	{
		PC->bShowMouseCursor = false;
		PC->SetInputMode(FInputModeGameOnly());
	}
}
