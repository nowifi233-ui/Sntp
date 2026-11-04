// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_EnemyAttack.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Animation/AnimMontage.h"

USnGA_EnemyAttack::USnGA_EnemyAttack()
{
	// The AI that activates this lives on the server only, and the swing's
	// damage must be authoritative. Clients replay the montage through the
	// ability system's own replication and watch the cues multicast.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// One reusable instance per enemy: an active instance IS the "mid-swing"
	// state (ASnEnemy::IsAttacking), and the engine refuses to re-activate an
	// active InstancedPerActor ability - so the enemy's per-frame retry cannot
	// stack a second swing on the running montage (see the class doc).
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool USnGA_EnemyAttack::IsValidAttackTarget(AActor* Target) const
{
	// The player team tag, added to the player's ASC at construction - the
	// mirror of the enemy tag every player-side query uses. Deliberately not
	// USnCombatLibrary::IsEnemy: that stays the player's question ("is this a
	// valid target for me") and keeps its four call sites (sphere core default,
	// projectile impact, FindNearestEnemy, homing) unchanged.
	const UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	return TargetASC && TargetASC->HasMatchingGameplayTag(SnGameplayTags::Combat_Team_Player);
}

void USnGA_EnemyAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !DamageEffectClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// The cooldown starts here, at activation - the same rule as the input
	// attack: a swing cut short (the target fled, the enemy died, the pool
	// reconfigured it) must still pay for itself, or a cancelled wind-up would
	// hand the enemy a free swing the very next frame. This application is what
	// arms the gate the next TryActivateAbility runs into.
	//
	// Both halves are checked first: a missing CooldownGameplayEffectClass or
	// empty CooldownTags is silent otherwise (the engine's gate short-circuits
	// on an empty tag container), and the enemy would simply swing again the
	// frame its montage ends. Name the cause instead of leaving that pace to be
	// reverse-engineered from the animation length. One line per swing, so a
	// misconfigured enemy reads as loud spam rather than silence.
	const FGameplayTagContainer* CooldownTagsPtr = GetCooldownTags();
	if (!CooldownGameplayEffectClass || !CooldownTagsPtr || CooldownTagsPtr->IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: no cooldown configured (CooldownGameplayEffectClass + CooldownTags) - the swing repeats as soon as the montage ends"), *GetName());
	}
	ApplyAttackCooldown(SourceASC, Avatar);

	// No montage configured: the damage resolves at the activation frame, so a
	// half-configured enemy still fights instead of standing there menacingly.
	if (!AttackMontage)
	{
		ExecuteSphereAttack(SourceASC, Avatar);
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
		return;
	}

	// Montage path: the damage waits for the animation's own hit frame and the
	// ability lives as long as the montage - the shared hit-frame lifecycle
	// (authority gate on the window, every montage exit ends the ability).
	PlayAttackMontageAndWaitForWindow(AttackMontage, MontagePlayRate, MontageStartSection);
}
