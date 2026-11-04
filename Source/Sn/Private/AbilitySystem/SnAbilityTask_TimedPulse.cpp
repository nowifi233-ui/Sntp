// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnAbilityTask_TimedPulse.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "TimerManager.h"

USnAbilityTask_TimedPulse* USnAbilityTask_TimedPulse::CreateTimedPulse(UGameplayAbility* OwningAbility, float InInitialDelay, float InIntervalSeconds, int32 InMaxTriggerCount)
{
	USnAbilityTask_TimedPulse* MyObj = NewAbilityTask<USnAbilityTask_TimedPulse>(OwningAbility);
	MyObj->InitialDelay = FMath::Max(0.f, InInitialDelay);
	MyObj->IntervalSeconds = InIntervalSeconds;
	MyObj->MaxTriggerCount = FMath::Max(0, InMaxTriggerCount);
	MyObj->PulseCount = 0;
	return MyObj;
}

void USnAbilityTask_TimedPulse::Activate()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		EndTask();
		return;
	}

	// Infinite mode cannot ride a zero interval (that would be a per-frame
	// loop, frame-rate dependent) - clamp to a sane floor instead of
	// degenerating.
	if (MaxTriggerCount == 0)
	{
		IntervalSeconds = FMath::Max(IntervalSeconds, 0.05f);
	}
	else if (IntervalSeconds <= 0.f)
	{
		// Finite count with no rhythm = zero-gap burst: every pulse fires
		// synchronously right here. The last FirePulse ends the task itself,
		// so this loop only touches the local counter after it returns (the
		// object is GC-deferred, the same convention the engine's
		// UAbilityTask_Repeat relies on).
		const int32 BurstCount = MaxTriggerCount;
		for (int32 i = 0; i < BurstCount; ++i)
		{
			FirePulse();
		}
		return;
	}

	if (InitialDelay > 0.f)
	{
		// First pulse after the delay; the interval loop starts from there
		// (a repeating timer with a per-tick offset needs a handover).
		World->GetTimerManager().SetTimer(LoopTimerHandle, this, &USnAbilityTask_TimedPulse::StartLoop, InitialDelay, /*bLoop=*/false);
	}
	else
	{
		StartLoop();
	}
}

void USnAbilityTask_TimedPulse::StartLoop()
{
	// First pulse (immediately, or once the initial delay elapsed), then arm
	// the repeating interval timer while pulses remain. If the first pulse
	// finished a finite task, FirePulse already ended it - KeepPulsing reads
	// only the count, which stops the timer arm.
	FirePulse();
	if (KeepPulsing())
	{
		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(LoopTimerHandle, this, &USnAbilityTask_TimedPulse::FirePulse, IntervalSeconds, /*bLoop=*/true);
		}
	}
}

void USnAbilityTask_TimedPulse::FirePulse()
{
	// The ability may have ended between scheduled fires (the timer is cleared
	// in OnDestroy, but a fire can be in flight) - end quietly instead of
	// broadcasting to a dead ability.
	if (!ShouldBroadcastAbilityTaskDelegates())
	{
		EndTask();
		return;
	}

	++PulseCount;
	OnPulse.Broadcast(PulseCount);

	if (MaxTriggerCount != 0 && PulseCount >= MaxTriggerCount)
	{
		OnFinished.Broadcast();
		EndTask();
	}
}

bool USnAbilityTask_TimedPulse::KeepPulsing() const
{
	return MaxTriggerCount == 0 || PulseCount < MaxTriggerCount;
}

void USnAbilityTask_TimedPulse::OnDestroy(bool AbilityIsEnding)
{
	// Single cleanup point for every exit path: task finished by count, task
	// ended by the ability going down (engine TaskOwnerEnded), or the world
	// tearing down. Clearing is idempotent.
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(LoopTimerHandle);
	}

	Super::OnDestroy(AbilityIsEnding);
}
