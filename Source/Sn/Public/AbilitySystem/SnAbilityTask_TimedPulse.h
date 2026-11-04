// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "SnAbilityTask_TimedPulse.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FSnTimedPulseDelegate, int32, PulseCount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FSnTimedPulseFinishedDelegate);

/**
 * Repeating metronome task for the timed-trigger GA family
 * (USnGA_TimedTrigger and its sustain / volley shells): fires OnPulse once per
 * interval and ends the task when the trigger GA stops it (EndAbility ->
 * engine TaskOwnerEnded, see UGameplayAbility::EndAbility) or when a finite
 * MaxTriggerCount is reached.
 *
 * The engine metronome (UAbilityTask_Repeat) only does finite counts with the
 * first action immediate - no initial delay and no infinite mode (the engine
 * source's own TODO: "Should we allow negative values to mean unlimited?").
 * This task adds both:
 * - MaxTriggerCount 0 = infinite (sustain shells); the interval is clamped to
 *   a sane minimum so infinite mode never degenerates into a per-frame loop.
 * - InitialDelay: first pulse after InitialDelay; 0 = first pulse immediately
 *   (the sustain aura's "first pulse fires on activation" rule), then every
 *   IntervalSeconds.
 * - IntervalSeconds <= 0 with a finite count = zero-gap burst: all pulses run
 *   synchronously on activation (volley configs that want no rhythm).
 *
 * Timer-based like the engine repeat task (not a ticking task), with the
 * timer cleared in OnDestroy - the ability system ends every task through
 * TaskOwnerEnded when the owning ability ends, so the cleanup is automatic
 * wherever the ability goes down.
 */
UCLASS()
class SN_API USnAbilityTask_TimedPulse : public UAbilityTask
{
	GENERATED_BODY()

public:
	/** Fired on every pulse with its 1-based index (1 = first pulse). */
	UPROPERTY(BlueprintAssignable)
	FSnTimedPulseDelegate OnPulse;

	/** Fired once when a finite MaxTriggerCount has been fully pulsed. */
	UPROPERTY(BlueprintAssignable)
	FSnTimedPulseFinishedDelegate OnFinished;

	/** Creates the task. MaxTriggerCount 0 = infinite (requires
	 *  IntervalSeconds > 0, clamped in Activate). */
	static USnAbilityTask_TimedPulse* CreateTimedPulse(UGameplayAbility* OwningAbility, float InInitialDelay, float InIntervalSeconds, int32 InMaxTriggerCount);

	virtual void Activate() override;
	virtual void OnDestroy(bool AbilityIsEnding) override;

private:
	/** Fires one pulse; a finite task ends itself on its last pulse. */
	void FirePulse();

	/** Arms the interval loop after the first pulse (or the initial delay). */
	void StartLoop();

	/** True while more pulses should come (infinite, or count not yet met). */
	bool KeepPulsing() const;

	float InitialDelay = 0.f;
	float IntervalSeconds = 1.f;

	/** 0 = infinite (sustain semantics). */
	int32 MaxTriggerCount = 0;

	/** Pulses fired so far. */
	int32 PulseCount = 0;

	FTimerHandle LoopTimerHandle;
};
