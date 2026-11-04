// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_Attack.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"

USnGA_Attack::USnGA_Attack()
{
	// One live instance per actor: the engine rejects a re-activation while
	// this one is active, so a spammed button cannot start a second swing on
	// top of the running montage (see the class doc).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void USnGA_Attack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// The cooldown starts here, at activation - not at the hit frame: the
	// montage spans many frames, and a swing cancelled (or an attacker killed)
	// mid wind-up must still pay for itself.
	ApplyAttackCooldown(SourceASC, Avatar);

	// No montage: the original behaviour - the damage body runs here, inside
	// the activation scope, on both the predicted client copy and the server's;
	// the shared prediction key dedups their cues (see ExecuteSphereAttack).
	if (!MontageToPlay)
	{
		ExecuteSphereAttack(SourceASC, Avatar);
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
		return;
	}

	// Montage path: the ability now lives as long as the animation and the
	// damage waits for the montage's own notify. The lifecycle that goes with
	// it - both tasks, the four montage-exit pins, the authority gate on the
	// hit frame - is shared with the other montage roles (USnGA_SphereAttack).
	PlayAttackMontageAndWaitForWindow(MontageToPlay, MontagePlayRate, MontageStartSection);
}
