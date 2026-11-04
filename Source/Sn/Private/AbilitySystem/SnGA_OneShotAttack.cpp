// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_OneShotAttack.h"

#include "AbilitySystemComponent.h"

USnGA_OneShotAttack::USnGA_OneShotAttack()
{
	// See the class doc: the engine one-shot API issues the activation on the
	// server with an empty prediction key, and a same-frame ability can't ride
	// the ServerInitiated client push either - ServerOnly is the only policy
	// where the grant -> activate -> remove lifecycle completes in one server
	// frame.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	// Fresh instance per use: a one-shot spec carries no state between uses,
	// and no lingering instance can ever block the next activation.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void USnGA_OneShotAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// The cooldown step the shared core no longer performs itself (see
	// ApplyAttackCooldown): this instance is ServerOnly, so the CD is applied
	// on the authority here, right before the damage body - same order and same
	// result as when the core applied it inline.
	ApplyAttackCooldown(SourceASC, Avatar);

	// The full swing -> damage -> hit feedback flow, gameplay cues included,
	// lives in the shared core - the server-only instance runs it once and
	// the core's cues multicast to every client (see ExecuteSphereAttack).
	ExecuteSphereAttack(SourceASC, Avatar);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
}
