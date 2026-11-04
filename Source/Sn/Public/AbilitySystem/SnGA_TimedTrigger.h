// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGameplayAbility.h"
#include "SnGA_TimedTrigger.generated.h"

/**
 * Timed-trigger ability family core (Abstract): the shared engine that fires
 * another configured ability (TriggeredAbilityClass) on a repeating schedule.
 * All rhythm state lives here; shells (USnGA_SustainTrigger = pulse forever
 * until the ability ends, USnGA_VolleyTrigger = release a fixed count then
 * end) only differ in constructor defaults and activation entry.
 *
 * Per-release model (design doc GA_TRIGGER_AND_SUMMON.md §2.1): this ability
 * PRE-GRANTS the child on its own ActivateAbility and each release is a plain
 * TryActivateAbility on that persistent spec - the same layer split as the
 * always-granted input attack. The child spec only exists while this ability
 * is active: EndAbility revokes it, so an unequip (spec removal ends the
 * active instance) or a cancel leaves nothing behind - paired grant/remove
 * bookkeeping lives in this core. A release whose TryActivateAbility fails
 * (child on its own cooldown, or an InstancedPerActor child still active from
 * the previous release) is skipped - the rhythm never blocks or queues.
 *
 * Rhythm comes from USnAbilityTask_TimedPulse (not a raw timer): the task
 * owns the metronome and is destroyed with the ability, so stopping the
 * schedule needs no manual timer bookkeeping. Children must be ServerOnly
 * single-frame bodies (the USnGA_OneShotAttack family): a child that stays
 * active past its frame would swallow the next release on the
 * concurrent-activation rejection. Children must NOT configure their own
 * per-release cooldown (the shell owns the rhythm), and their cue-based
 * visuals multicast with an empty prediction key - one burst per machine.
 *
 * The engine cooldown fields (CooldownGameplayEffectClass, on the shell
 * asset) work as usual: CommitAbility applies the cooldown when the shell
 * activates, gating whole runs of the rhythm, never individual releases.
 */
UCLASS(Abstract)
class SN_API USnGA_TimedTrigger : public USnGameplayAbility
{
	GENERATED_BODY()

public:
	USnGA_TimedTrigger();

	/** Ability released on every pulse. Must be a ServerOnly single-frame body
	 *  (the USnGA_OneShotAttack family, or the summon family - see class doc);
	 *  an InstancedPerActor child blocks its next release while active, which
	 *  is exactly the summon-stream semantics (a live summon keeps the next
	 *  release waiting until it dies). */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger")
	TSubclassOf<USnGameplayAbility> TriggeredAbilityClass;

	/** Seconds before the first release; 0 = first release immediately on
	 *  activation. */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger", meta = (ClampMin = "0.0"))
	float InitialDelay = 0.f;

	/** Seconds between releases (0 with a finite MaxTriggerCount = zero-gap
	 *  burst: every release fires synchronously on activation). */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger", meta = (ClampMin = "0.0"))
	float IntervalSeconds = 1.f;

	/** Total releases before the ability ends itself; 0 = infinite (the ability
	 *  keeps releasing until it is ended from outside: unequip / cancel /
	 *  level travel). */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger", meta = (ClampMin = "0.0"))
	int32 MaxTriggerCount = 0;

	/** Grants the child for the whole active window, starts the metronome. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Revokes the child grant (paired with ActivateAbility); the task is
	 *  already gone - the engine ended it when the ability ended. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Description: "每 {N} 秒释放一次：…" / "连续释放 {N} 次：…" prefix composed
	 *  with the child's own description text - the numbers live in one place,
	 *  the child asset, and every trigger referencing it stays in sync. */
	virtual FText GetDescription_Implementation(int32 Level) override;

private:
	/** One release: TryActivateAbility on the pre-granted child handle.
	 *  Failure is a silent skip by design (see class doc). */
	void ReleasePulse();

	UFUNCTION()
	void HandleTimedPulse(int32 PulseCount);

	/** Finite MaxTriggerCount reached: end the trigger (and with it, the child
	 *  grant) without counting as a cancel. */
	UFUNCTION()
	void HandleTimedPulseFinished();

	/** The child spec granted while this ability is active (paired with the
	 *  EndAbility removal). */
	FGameplayAbilitySpecHandle TriggeredAbilityHandle;
};
