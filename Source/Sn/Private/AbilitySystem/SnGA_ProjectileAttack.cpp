// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_ProjectileAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnGA_DamageSkill.h"
#include "Combat/ASnProjectile.h"
#include "Combat/USnCombatLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

USnGA_ProjectileAttack::USnGA_ProjectileAttack()
{
	// See the class doc: every current entry (use-item one-shot, timed-trigger
	// child, equip grant) activates on the server with an empty prediction
	// key, and the launch ends in the same frame - ServerOnly is the only
	// policy where the lifecycle completes cleanly, and the spawned
	// projectile (replicated actor) carries the visuals to every client.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	// Fresh instance per launch: no state lives between uses, and no lingering
	// instance can block the next pulse of a trigger shell.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

void USnGA_ProjectileAttack::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* SourceASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !DamageEffectClass || !ProjectileClass)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Damage decided NOW, applied at impact: pre-build the spec the projectile
	// will carry (MakeDamageSpec - the same spec ApplyDamageToTarget applies
	// instantly, kept for the delayed hit). The element flag and the
	// instigator ride on the spec's context, so kill credit and the ExecCalc
	// resolution work at impact time without the projectile storing numbers.
	FSnDamageResult Damage = USnCombatLibrary::CalculateDamage(SourceASC, DamageType);
	Damage.Damage = USnGA_DamageSkill::ScaleDamageByLevel(Damage.Damage, Multiplier, PerLevelGrowth, GetAbilityLevel());
	const FGameplayEffectSpecHandle DamageSpec = USnCombatLibrary::MakeDamageSpec(SourceASC, DamageEffectClass, Avatar, Damage.Damage, DamageType, GetAbilityLevel());
	if (!DamageSpec.IsValid())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Fire where the player is looking: the aim is READ off the controller
	// (GetAvatarAimRotation) - the body faces its run direction, not the camera,
	// and a launch deliberately does not turn it (no body snap, so no camera drag
	// on the launch frame). Companions and other AI attackers keep firing along
	// their own facing, the yaw their AI set.
	const FVector AimDirection = GetAvatarAimRotation(Avatar).Vector();

	// Launch from MuzzleOffset in front of the attacker, along the aim. With
	// homing on and a target in range, aim at the target so the missile visibly
	// banks onto it (missile turn strength is the projectile blueprint's physics;
	// the GA only decides whether to hunt).
	const FVector MuzzleLocation = Avatar->GetActorLocation() + AimDirection * MuzzleOffset;
	FVector LaunchDirection = AimDirection;
	AActor* HomingTarget = nullptr;
	if (bHoming)
	{
		HomingTarget = USnCombatLibrary::FindNearestEnemy(this, MuzzleLocation, HomingSearchRadius);
		if (HomingTarget)
		{
			LaunchDirection = (HomingTarget->GetActorLocation() - MuzzleLocation).GetSafeNormal();
		}
	}

	UWorld* World = GetWorld();
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = Avatar;
	SpawnParams.Instigator = Cast<APawn>(Avatar);
	ASnProjectile* Projectile = World->SpawnActor<ASnProjectile>(ProjectileClass, MuzzleLocation, LaunchDirection.Rotation(), SpawnParams);
	if (!Projectile)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// The projectile now owns the rest of the family body: engine PMC flight
	// (its spawn rotation already aimed it - the engine resolves the initial
	// velocity from InitialSpeed along the spawn rotation), server-side hit
	// resolution against the pre-built spec, the hit cue at the impact point,
	// and the wall/lifetime cleanup (see ASnProjectile). The homing target is
	// attached at runtime - the engine rule (HomingTargetComponent must be
	// set after the projectile is spawned).
	Projectile->InitializeProjectile(DamageSpec, Avatar, SourceASC, HitCueTag);
	if (HomingTarget)
	{
		Projectile->SetHomingTarget(HomingTarget->GetRootComponent());
	}

	// Launch cue at the muzzle, miss semantics (RawMagnitude 0: nothing was
	// damaged by the launch itself - the notify's hit-shake gate stays off).
	if (SwingCueTag.IsValid())
	{
		FGameplayCueParameters LaunchParams;
		LaunchParams.Location = MuzzleLocation;
		LaunchParams.Normal = LaunchDirection;
		LaunchParams.RawMagnitude = 0.f;
		LaunchParams.Instigator = Avatar;
		SourceASC->ExecuteGameplayCue(SwingCueTag, LaunchParams);
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
}

FText USnGA_ProjectileAttack::GetDescription_Implementation(int32 Level)
{
	// The shared damage-family element clause wrapped in the family's own
	// sentences; homing skills say so up front.
	return FText::Format(
		FText::FromString(bHoming ? TEXT("发射一枚追踪弹，追踪最近的敌人，命中时造成 {0}") : TEXT("发射一枚弹道，命中时对敌人造成 {0}")),
		USnGA_DamageSkill::BuildDamageClause(Level, Multiplier, PerLevelGrowth, DamageType));
}
