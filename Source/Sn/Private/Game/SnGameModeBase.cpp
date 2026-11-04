// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SnGameModeBase.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "Characters/SnCharacter.h"
#include "Engine/World.h"
#include "Game/SnGameState.h"
#include "Game/SnHUD.h"
#include "Game/SnSettings.h"
#include "Game/USnDifficultyLibrary.h"
#include "TimerManager.h" // FTimerManager / FTimerDelegate
#include "Game/USnGameInstance.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"

ASnGameModeBase::ASnGameModeBase()
{
	DefaultPawnClass = ASnCharacter::StaticClass();
	PlayerControllerClass = ASnPlayerController::StaticClass();
	PlayerStateClass = ASnPlayerState::StaticClass();
	HUDClass = ASnHUD::StaticClass();

	// Session-scoped difficulty (RoR2): the tier lives on the GameState so
	// every player in the run reads the same value, replicated from the server.
	GameStateClass = ASnGameState::StaticClass();
}

void ASnGameModeBase::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	// Super already spawned + possessed the pawn: InitAbilityActorInfo has run
	// on the player state, so save-restored attributes and the restored
	// difficulty tier are in place. Running the zone hook here makes the
	// difficulty GE land AFTER the base values - never before them.
	if (APlayerController* PC = Cast<APlayerController>(NewPlayer))
	{
		HandlePlayerEnteredZone(PC);
	}
}

void ASnGameModeBase::Logout(AController* Exiting)
{
	// Before Super, which hands the exit to the game session and the BP event:
	// this is the last moment the leaving player's state is still intact
	// (AController::Destroyed calls us, then cleans the state up), and taking
	// the capture is the whole reason this hook exists. It also covers the two
	// ways out - travelling home to the main menu and quitting the game
	// outright - through one path.
	if (USnGameInstance* GI = GetGameInstance<USnGameInstance>())
	{
		GI->SaveDepartedCharacter(Exiting ? Exiting->GetPlayerState<ASnPlayerState>() : nullptr);
	}

	Super::Logout(Exiting);
}

void ASnGameModeBase::HandlePlayerEnteredZone(APlayerController* PC)
{
	ASnPlayerState* PS = PC ? PC->GetPlayerState<ASnPlayerState>() : nullptr;
	if (!PS)
	{
		return;
	}
	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	if (ZoneType == ESnZoneType::Combat)
	{
		// Apply the session's difficulty tier (no-op when the tier is unknown or
		// the table is not configured). Idempotent: any previous difficulty
		// effect is removed inside, so respawns re-apply cleanly. Bind the
		// change handler once too: the menu picks a tier after spawn, and the
		// handler re-applies the player-side effect immediately instead of
		// waiting for the next respawn.
		if (ASnGameState* SnGameState = GetGameState<ASnGameState>())
		{
			if (!bDifficultyBound)
			{
				SnGameState->OnDifficultyChanged.AddDynamic(this, &ASnGameModeBase::HandleDifficultyChanged);
				bDifficultyBound = true;
			}
			if (const FSnDifficultyRow* Row = USnDifficultyLibrary::FindDifficultyRow(SnGameState->GetDifficulty()))
			{
				USnDifficultyLibrary::ApplyDifficultyToPlayer(ASC, *Row);
			}
		}

		// Death ends the run for the whole team (see HandlePlayerDied). Re-bound
		// on every spawn instead of behind a flag: this hook runs once per
		// player per respawn and AddDynamic does not dedupe, so RemoveDynamic
		// first is what keeps the count at one - the project's idempotent-bind
		// pattern (SnEnemy::BindAttributeCallbacks).
		if (USnAttributeSet* SnSet = PS->GetAttributeSet())
		{
			SnSet->OnDeath.RemoveDynamic(this, &ASnGameModeBase::HandlePlayerDied);
			SnSet->OnDeath.AddDynamic(this, &ASnGameModeBase::HandlePlayerDied);
		}
	}
	else if (ZoneType == ESnZoneType::Hub)
	{
		// Back to base values (attributes revert to base + equipment). The
		// run's tier is NOT reset here: it persists in the save (snapshot
		// Difficulty, restored by the player state on entry) and the menu can
		// change it at any time.
		USnDifficultyLibrary::RemoveDifficultyFromPlayer(ASC);

		// Hub entry also runs the single-player intro tutorial (once ever -
		// milestone-guarded inside).
		MaybePlayIntroTutorial(PC);
	}
}

void ASnGameModeBase::MaybePlayIntroTutorial(APlayerController* PC)
{
	// Single-player only: the popup locks game input (UI-only mode) and would
	// hijack a LAN co-op guest's controls - the tutorial belongs to solo runs
	// (multiplayer hosts/guests learn from their teammates instead).
	if (!PC || GetNetMode() != NM_Standalone)
	{
		return;
	}

	USnGameInstance* SnGI = Cast<USnGameInstance>(GetWorld()->GetGameInstance());
	if (!SnGI)
	{
		return;
	}

	// Once ever, and it persists in the save: the milestone IS the tutorial
	// (the first row's name doubles as the milestone ID). Written BEFORE the
	// popup opens, so skipping with Escape still counts as taught - it never
	// re-plays on later hub entries or through the save.
	static const FName TutorialMilestone = TEXT("Tutorial.HubIntro");
	if (SnGI->HasMilestone(TutorialMilestone))
	{
		return;
	}
	SnGI->AddMilestone(TutorialMilestone);

	// Small beat after spawn so the camera settles into the fresh pawn before
	// the popup grabs input. The GameMode owns the timer: it dies with this
	// level, which is exactly the tutorial's scope (the world also tears it
	// down on travel).
	GetWorldTimerManager().SetTimer(IntroTutorialTimerHandle,
		FTimerDelegate::CreateWeakLambda(this,
			[this, WeakPC = TWeakObjectPtr<APlayerController>(PC)]()
			{
				// The milestone constant is a function-static - reachable from
				// the lambda without capturing it.
				if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(WeakPC.Get()))
				{
					// Tutorial: pause behind the popup (single-player only -
					// released by the PC when the popup closes).
					SnPC->PlayDialogue(TutorialMilestone, /*bPauseGame=*/true);
				}
			}),
		1.0f, false);
}

void ASnGameModeBase::HandleDifficultyChanged(FName NewDifficulty)
{
	if (ZoneType != ESnZoneType::Combat)
	{
		return;
	}
	const FSnDifficultyRow* Row = USnDifficultyLibrary::FindDifficultyRow(NewDifficulty);
	if (!Row)
	{
		return; // Stale name (table changed) - keep whatever is applied.
	}

	// Session-wide tier: every player in the zone re-applies (remove-first
	// inside, so this converges to a single multiplier layer). Players whose
	// ASC is not ready yet are skipped - the RestartPlayer hook applies to
	// them on spawn.
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		ASnPlayerState* PS = PC ? PC->GetPlayerState<ASnPlayerState>() : nullptr;
		UAbilitySystemComponent* ASC = PS ? PS->GetAbilitySystemComponent() : nullptr;
		if (ASC)
		{
			USnDifficultyLibrary::ApplyDifficultyToPlayer(ASC, *Row);
		}
	}
}

void ASnGameModeBase::HandlePlayerDied(AActor* Killer)
{
	// Combat zones only. The hub never binds this (the binding lives in the
	// combat arm of HandlePlayerEnteredZone), and the guard also covers a
	// notification arriving late in a frame that is already leaving.
	if (ZoneType != ESnZoneType::Combat)
	{
		return;
	}

	// Rescue on EVERY notification, not just the first. A team wipe is several
	// deaths inside one frame and only the first one gets past the guard below:
	// the rest would otherwise travel home with Health == 0, which is the one
	// value that must never reach the snapshot (see TravelToHub).
	ReviveDeadPlayers();

	if (bDeathReturnPending)
	{
		return;
	}
	bDeathReturnPending = true;

	// Defer the travel by a tick. RequestTravel snapshots on its way in, and a
	// teammate killed by the same blast may not have run their own handler yet
	// - by next tick every death from this frame has been revived, so the save
	// never records a corpse.
	GetWorldTimerManager().SetTimerForNextTick(
		FTimerDelegate::CreateWeakLambda(this, [this]() { TravelToHub(); }));
}

void ASnGameModeBase::ReviveDeadPlayers()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Server-side attribute writes (this is the GameMode): they reach the
	// clients through ordinary attribute replication, so no RPC of our own.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		ASnPlayerState* PS = It->Get() ? It->Get()->GetPlayerState<ASnPlayerState>() : nullptr;
		USnAttributeSet* SnSet = PS ? PS->GetAttributeSet() : nullptr;
		if (!SnSet || SnSet->GetHealth() > 0.f)
		{
			continue;
		}

		// Re-arm the one-shot death flag FIRST, then heal: bIsDead is what
		// makes OnDeath fire once per life (leaving it set would make the
		// player death-proof in every combat level that follows), and it is
		// also what refuses Health increases - healing a latched corpse is a
		// no-op, which would send the team home at zero health, the one value
		// that must never reach the snapshot.
		SnSet->ResetDeath();
		SnSet->SetHealth(SnSet->GetMaxHealth());
	}
}

void ASnGameModeBase::TravelToHub()
{
	// The hub is the start level - the same key a new game travels to, so
	// "where is home" has one answer and no second table to keep in sync.
	const FString HubLevelName = USnSettings::Get()->GetStartLevelName();
	if (HubLevelName.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnGameModeBase::TravelToHub: USnSettings 的 StartLevel 未配置，死亡回城被跳过"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Any player's controller can drive it: RequestTravel is a whole-machine
	// travel that walks every client through the barrier, so ONE caller moves
	// the team and "everyone comes along" needs no loop of its own. Deliberately
	// not CloseOnlineSession - the hub is another level inside the run, not the
	// way out of the session.
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(It->Get()))
		{
			SnPC->RequestTravel(HubLevelName);
			return;
		}
	}
}
