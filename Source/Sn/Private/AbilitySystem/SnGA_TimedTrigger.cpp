// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_TimedTrigger.h"

#include "AbilitySystem/SnAbilityTask_TimedPulse.h"
#include "AbilitySystemComponent.h"

USnGA_TimedTrigger::USnGA_TimedTrigger()
{
	// The rhythm (child grant + releases) must be server-authoritative; the
	// child's visuals multicast on their own via gameplay cues.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void USnGA_TimedTrigger::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	if (!SourceASC || !TriggeredAbilityClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Same pattern as the summon ability: activation is gated by
	// CanActivateAbility, so a commit failure here means something invalidated
	// the ability mid-activation - bail rather than run a partial schedule.
	// (CommitAbility also applies the shell's engine cooldown asset when one
	// is configured, gating whole runs of the rhythm.)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Pre-grant the child once for the whole active window: releases are then
	// plain TryActivateAbility calls (no per-release grant/remove churn, and
	// the child spec is visible to the owning client). EndAbility revokes it -
	// see the paired removal there.
	if (!TriggeredAbilityHandle.IsValid())
	{
		TriggeredAbilityHandle = SourceASC->GiveAbility(FGameplayAbilitySpec(TriggeredAbilityClass, GetAbilityLevel()));
	}

	// The task owns the metronome (interval / initial delay / finite count)
	// and is destroyed with this ability - no manual timer bookkeeping.
	USnAbilityTask_TimedPulse* PulseTask = USnAbilityTask_TimedPulse::CreateTimedPulse(this, InitialDelay, IntervalSeconds, MaxTriggerCount);
	PulseTask->OnPulse.AddDynamic(this, &USnGA_TimedTrigger::HandleTimedPulse);
	PulseTask->OnFinished.AddDynamic(this, &USnGA_TimedTrigger::HandleTimedPulseFinished);
	PulseTask->ReadyForActivation();
}

void USnGA_TimedTrigger::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// Paired with the grant in ActivateAbility: the child only exists inside
	// this ability's active window. Every exit path lands here (natural
	// finish, cancel, spec removal on unequip - the engine ends the active
	// instance) and the engine already destroyed the task, so this is the
	// single point that keeps the pre-grant model leak-free.
	if (TriggeredAbilityHandle.IsValid())
	{
		if (UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr)
		{
			SourceASC->ClearAbility(TriggeredAbilityHandle);
		}
		TriggeredAbilityHandle = FGameplayAbilitySpecHandle();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FText USnGA_TimedTrigger::GetDescription_Implementation(int32 Level)
{
	// Compose the shell's rhythm text with the child's own description: the
	// numbers live in the child asset, so a trigger tooltip reads
	// "每 5 秒释放一次：<child description>" and tweaking the child's values
	// updates every trigger that references it.
	FText ChildDescription;
	if (TriggeredAbilityClass)
	{
		if (USnGameplayAbility* ChildCDO = GetMutableDefault<USnGameplayAbility>(TriggeredAbilityClass))
		{
			ChildDescription = ChildCDO->GetDescription(Level);
		}
	}

	if (MaxTriggerCount > 0)
	{
		return FText::Format(INVTEXT("连续释放 {0} 次：{1}"), FText::AsNumber(MaxTriggerCount), ChildDescription);
	}

	return FText::Format(INVTEXT("每 {0} 秒释放一次：{1}"), FText::FromString(FString::Printf(TEXT("%g"), IntervalSeconds)), ChildDescription);
}

void USnGA_TimedTrigger::ReleasePulse()
{
	// One child release per pulse. A failed activation (child misconfigured
	// with its own cooldown, or an InstancedPerActor child still active from
	// the previous pulse) skips the pulse silently - the rhythm never blocks
	// or queues.
	if (TriggeredAbilityHandle.IsValid())
	{
		if (UAbilitySystemComponent* SourceASC = GetAbilitySystemComponentFromActorInfo())
		{
			SourceASC->TryActivateAbility(TriggeredAbilityHandle);
		}
	}
}

void USnGA_TimedTrigger::HandleTimedPulse(int32 PulseCount)
{
	// (PulseCount is informational; a future shell may count or branch on it.)
	ReleasePulse();
}

void USnGA_TimedTrigger::HandleTimedPulseFinished()
{
	// A finite schedule is done: end the trigger (and with it, the child grant
	// in EndAbility) - never as a cancel, so unequip-on-finished listeners see
	// a clean completion.
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
}
