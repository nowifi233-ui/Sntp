// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TimerHandle.h" // FTimerHandle
#include "GameFramework/GameModeBase.h"
#include "SnGameModeBase.generated.h"

/** What kind of zone this level is. The GameMode uses it to run the zone
 *  entry hook (HandlePlayerEnteredZone): combat zones apply the player's
 *  difficulty effect, hub zones strip it. Set per level via a Blueprint
 *  subclass in the World Settings (UE's native per-level GameMode). */
UENUM(BlueprintType)
enum class ESnZoneType : uint8
{
	None,
	Hub,
	Combat
};

/**
 * Base GameMode: zones a level as Hub / Combat so every player who enters
 * gets the zone's initialization (player-side difficulty GE) at the exact
 * point where the player state is fully restored (RestartPlayer has run
 * Super, so possession + InitAbilityActorInfo + save restore already
 * happened).
 */
UCLASS()
class SN_API ASnGameModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ASnGameModeBase();

	/** The zone this level belongs to (Hub / Combat). Configured on the
	 *  level's GameMode Blueprint subclass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zone")
	ESnZoneType ZoneType = ESnZoneType::None;

protected:
	/** Runs after the player was spawned/possessed/restored: the authoritative
	 *  moment to apply the zone's initialization. GameMode code runs on the
	 *  server only, so the difficulty GE is applied server-side and reaches
	 *  clients through GE replication (Mixed mode). */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** Zone entry hook: apply or strip the player-side difficulty effect
	 *  (combat zone: from the GameState's tier; hub zone: remove). */
	virtual void HandlePlayerEnteredZone(APlayerController* PC);

	/** A player left the room (the engine calls this from
	 *  AController::Destroyed, before the state is cleaned up). The host's
	 *  roster takes a last capture of them and writes the file - without it a
	 *  guest who quits instead of travelling would lose everything since the
	 *  last travel. Server-only by construction: a client machine has no game
	 *  mode at all (UWorld::SetGameMode is gated on !IsNetMode(NM_Client)). */
	virtual void Logout(AController* Exiting) override;

private:
	/** Single-player intro tutorial: on a hub entry, opens the dialogue at
	 *  "Tutorial.HubIntro" the very first time the player enters the hub
	 *  (milestone-guarded, and the milestone is written the moment the
	 *  tutorial triggers - skipping with Escape still counts as taught, it
	 *  never re-plays). LAN co-op never plays it (see the body). */
	void MaybePlayIntroTutorial(APlayerController* PC);

	/** The intro tutorial's delayed-open timer (beat after spawn so the
	 *  camera settles first). Owned by the GameMode: it dies with this level,
	 *  which is exactly the tutorial's scope. */
	FTimerHandle IntroTutorialTimerHandle;

	/** GameState::OnDifficultyChanged handler (bound once by the combat-zone
	 *  branch of HandlePlayerEnteredZone): re-applies the tier's player-side
	 *  effect to every player in the zone immediately. The menu picks a tier
	 *  after spawn - the RestartPlayer hook alone would only apply on the
	 *  next respawn. */
	UFUNCTION()
	void HandleDifficultyChanged(FName NewDifficulty);

	/** True once OnDifficultyChanged is bound (co-op players entering the zone
	 *  later must not bind it a second time). */
	bool bDifficultyBound = false;

	/** A player died in a combat zone: the run ends for the whole team - every
	 *  dead player is revived, then the team travels back to the hub. Bound to
	 *  EACH player's AttributeSet::OnDeath by the combat arm of
	 *  HandlePlayerEnteredZone. Note the parameter is the KILLER, not the
	 *  victim (that is FSnDeathDelegate's shape), so the victim is found by
	 *  scanning for zero health - which also catches a teammate killed by the
	 *  same blast. */
	UFUNCTION()
	void HandlePlayerDied(AActor* Killer);

	/** Revives every player sitting at zero health and re-arms their one-shot
	 *  death flag. Kept separate from HandlePlayerDied because it must run on
	 *  every death notification, including the ones the pending guard turns
	 *  away: a team wipe is several deaths in one frame. */
	void ReviveDeadPlayers();

	/** The snapshot-safe half of the death return: sends the team home through
	 *  the normal travel barrier. Runs one tick AFTER the death so the rest of
	 *  that frame's damage has resolved (see HandlePlayerDied). */
	void TravelToHub();

	/** True once the death return has started. Later deaths still revive, but
	 *  must not re-enter the travel barrier - a second RequestTravel would
	 *  re-send ClientPrepareTravel and corrupt the confirmation count. Never
	 *  reset: this GameMode dies with the level, which is one death return per
	 *  visit by construction. */
	bool bDeathReturnPending = false;
};
