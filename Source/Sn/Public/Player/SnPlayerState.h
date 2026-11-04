// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Delegates/DelegateBase.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "SnPlayerState.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
class UGameplayAbility;
class USnInventoryComponent;
struct FSnCharacterSave;
struct FSnRunSave;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnLevelChangedDelegate, int32, NewLevel);

/**
 * Owns the AbilitySystemComponent and the primary attribute set.
 * Attributes live here so they survive pawn death and respawn.
 */
UCLASS()
class SN_API ASnPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ASnPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End of IAbilitySystemInterface

	/** Returns the primary attribute set (Health, MaxHealth, MP, PhysicalDamage). */
	UFUNCTION(BlueprintPure, Category = "AbilitySystem")
	USnAttributeSet* GetAttributeSet() const { return AttributeSet; }

	/** Returns the player's inventory + equipment component. */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	USnInventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** Placeholder: the player's XP gain multiplier (RoR2: one pickup is
	 *  absorbed, the whole team gains XP - each member settles their own share
	 *  with their own multiplier). Constant 1.0 for now; designed to become an
	 *  attribute later. */
	UFUNCTION(BlueprintPure, Category = "Leveling")
	float GetXPGetMultiplier() const { return 1.f; }

	/** Broadcast when the player gains a level. */
	UPROPERTY(BlueprintAssignable, Category = "Leveling")
	FSnLevelChangedDelegate OnLevelChanged;

	/** Initializes the ASC (owner = this player state, avatar = pawn) and applies default attributes. Safe to call repeatedly. */
	void InitAbilityActorInfo(AActor* AvatarActor);

	/** Copies the live character (attributes + inventory + equipment + hotbar)
	 *  into OutCharacter. Called by the save layer right before writing to disk,
	 *  and by the travel capture for EVERY player's state. */
	void FillCharacterSnapshot(FSnCharacterSave& OutCharacter) const;

	/** Copies the room's world-derived fields (level name, difficulty, and
	 *  where THIS state's pawn stands) into OutRun. The rest of the run - floor,
	 *  milestones, devices - is owned and kept live by the GameInstance. Only
	 *  the local player's state is captured this way: the room is this
	 *  machine's. */
	void FillRunSnapshot(FSnRunSave& OutRun) const;

	/** Applies whatever the room's roster holds for this state - hooked at the
	 *  end of InitAbilityActorInfo (so defaults land first, then the character
	 *  overwrites them) and again when a late profile report arrives (see
	 *  SetProfileID). Server-only (see the definition), no-op before the ASC
	 *  exists, and a no-op when the roster has nothing under this player's key -
	 *  a guest who just joined has no history in this room and keeps the
	 *  defaults. Runs at most once per state (bCharacterRestored). */
	void TryRestoreFromSave();

	/** The profile this player is (USnGameInstance's player ID, reported by the
	 *  controller - see ASnPlayerController::ServerSetPlayerName). Empty until
	 *  that report lands. It is the key the host files this player's character
	 *  under in the room's roster, so every state can find its own entry in the
	 *  next world; replicated so the owning machine can read back which profile
	 *  the server accepted for it (its own report can be rejected - see the
	 *  duplicate check in ServerSetPlayerName). */
	UFUNCTION(BlueprintPure, Category = "Sn|Save")
	FString GetProfileID() const { return ProfileID; }

	/** Server-only: binds the profile this player reported (validated by the
	 *  caller) and retries the restore - the report and the ASC initialization
	 *  are separate streams, and on a travel the report necessarily lands LAST
	 *  for a remote player (the client can only report once it possesses the
	 *  pawn the server just spawned, so this state already tried - and found no
	 *  key - a round trip earlier). Looking the roster up again costs nothing
	 *  and consumes nothing; bCharacterRestored is what stops a second
	 *  application. */
	void SetProfileID(const FString& InProfileID);

protected:
	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, Category = "AbilitySystem")
	TObjectPtr<USnAttributeSet> AttributeSet;

	/** Inventory + equipment. Lives here (like the ASC) so it survives respawns. */
	UPROPERTY(VisibleAnywhere, Category = "Inventory")
	TObjectPtr<USnInventoryComponent> InventoryComponent;

	/** The profile this player is (see GetProfileID). The server writes it when
	 *  the controller reports; replicated so the owning machine can read back
	 *  what the server holds for it - on a listen server the guest's own machine
	 *  knows its profile locally, but the two can drift (a guest who bound an ID
	 *  after joining), and what is in the session map is the server's answer. */
	UPROPERTY(Replicated)
	FString ProfileID;

	/** Gameplay effect applied on init to set up default attribute values (e.g. GE_InitAttributes). */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayEffect> DefaultAttributesEffect;

	/** Infinite MP regeneration effect applied once per ASC lifetime (e.g.
	 *  GE_ManaRegen: AttributeBased modifier on MP, sourced from MPRegen).
	 *  Guarded by bManaRegenApplied - respawns re-enter InitAbilityActorInfo
	 *  and must not stack another instance. */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayEffect> ManaRegenEffectClass;

	/** Infinite health regeneration effect applied once per ASC lifetime (e.g.
	 *  GE_HealthRegen: Infinite + Period, AttributeBased modifier on Health
	 *  sourced from HealthRegen). Leave the AttributeBased snapshot OFF so
	 *  equipment raising HealthRegen later is picked up on the next period.
	 *  Guarded by bHealthRegenApplied for the same reason as the mana one.
	 *  Regeneration stops at zero health on its own - the attribute set refuses
	 *  Health increases while the death latch is set (see PreAttributeChange). */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayEffect> HealthRegenEffectClass;

	/** Ability granted to the player on init (e.g. BP_GA_Attack). */
	UPROPERTY(EditDefaultsOnly, Category = "AbilitySystem")
	TSubclassOf<UGameplayAbility> AttackAbilityClass;

	/** MaxHealth gained per level-up (Health gets the same amount - leveling heals). */
	UPROPERTY(EditDefaultsOnly, Category = "Leveling")
	float LevelUpHealthGain = 20.f;

	/** PhysicalDamage gained per level-up (field keeps the old name so the
	 *  configured value stays valid). */
	UPROPERTY(EditDefaultsOnly, Category = "Leveling")
	float LevelUpAttackGain = 2.f;

	/** XP attribute changed: run the level-up check. */
	void HandleXPChanged(const FOnAttributeChangeData& Data);

	/** Consumes XP against the level curve, possibly several levels at once. */
	void CheckForLevelUp();

private:
	/** Applies a captured character: the base attributes first (level before
	 *  XP), then inventory/grid/hotbar/wallet. */
	void RestoreFromCharacter(const FSnCharacterSave& InCharacter);

	/** Applies the run half of a restore: the world's difficulty tier, and -
	 *  on a continue-game entry only - the saved position. */
	void RestoreFromRun(const FSnRunSave& InRun);

	FDelegateHandle XPChangeDelegateHandle;

	/** Guards the one-shot mana regen apply (InitAbilityActorInfo re-enters on
	 *  respawns and possession changes). */
	bool bManaRegenApplied = false;

	/** Same guard for the health regeneration effect. */
	bool bHealthRegenApplied = false;

	/** True once InitAbilityActorInfo has run to completion (defaults applied,
	 *  abilities granted, ASC pointed at the pawn). TryRestoreFromSave refuses to
	 *  run before that - restoration overwrites the defaults, so the order
	 *  matters - which is also what makes a late profile report (SetProfileID)
	 *  safe to use as a second restore trigger. */
	bool bAbilitySystemInitialized = false;

	/** True once this state has applied a character from the room's roster.
	 *  The roster itself is never consumed - it is the room's record, and every
	 *  level switch reads from it again - so the once-only rule lives here, on
	 *  the one object whose lifetime matches it exactly: a respawn re-enters the
	 *  restore path on the SAME state (must not re-grant and re-place what the
	 *  player already has), while a travel builds a NEW state (must apply it
	 *  again).
	 *
	 *  Not replicated: only the authority ever restores. */
	bool bCharacterRestored = false;

	/** The most recent runtime-constructed level-up effect. CheckForLevelUp
	 *  builds the effect on the fly and applies it synchronously - this strong
	 *  reference keeps it alive for the whole apply, and the replacement on the
	 *  next level-up lets GC reap the previous one. Without it, only the
	 *  synchronous execution window keeps the object alive. */
	UPROPERTY()
	TObjectPtr<UGameplayEffect> LevelUpEffect;
};
