// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_SphereAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Animation/AnimMontage.h"
#include "Combat/ASnDestructible.h"
#include "Combat/USnCombatLibrary.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameplayEffect.h"
#include "Kismet/KismetSystemLibrary.h"

void USnGA_SphereAttack::ApplyAttackCooldown(UAbilitySystemComponent* SourceASC, AActor* Avatar)
{
	if (!SourceASC || !CooldownGameplayEffectClass)
	{
		return;
	}

	const UGameplayEffect* CooldownCDO = CooldownGameplayEffectClass->GetDefaultObject<UGameplayEffect>();
	FGameplayEffectContextHandle CDContext = SourceASC->MakeEffectContext();
	CDContext.AddSourceObject(Avatar);
	const FGameplayEffectSpecHandle CDSpec = SourceASC->MakeOutgoingSpec(CooldownGameplayEffectClass, GetAbilityLevel(), CDContext);
	if (!CDSpec.IsValid())
	{
		return;
	}

	// 5.7: the duration magnitude is read via GetStaticMagnitudeIfPossible and
	// written via SetDuration (SetDurationMagnitude no longer exists).
	// SetDuration locks the value so nothing recomputes it from the GE's
	// magnitude; a non-static duration (e.g. SetByCaller) falls through and the
	// spec applies with the engine-computed duration.
	float BaseCooldown = 0.f;
	if (CooldownCDO && CooldownCDO->DurationMagnitude.GetStaticMagnitudeIfPossible(GetAbilityLevel(), BaseCooldown) && BaseCooldown > 0.f)
	{
		const float AttackSpeed = SourceASC->GetNumericAttribute(USnAttributeSet::GetAttackSpeedAttribute());
		CDSpec.Data->SetDuration(BaseCooldown / (1.f + FMath::Max(AttackSpeed, 0.f)), /*bLockDuration=*/true);
	}
	SourceASC->ApplyGameplayEffectSpecToSelf(*CDSpec.Data.Get());
}

bool USnGA_SphereAttack::IsValidAttackTarget(AActor* Target) const
{
	// The player-facing rule (see the header): anything carrying the enemy team
	// tag. Deliberately the library's own query and not a re-implementation, so
	// the default path can never drift from the projectiles' / homing's.
	return USnCombatLibrary::IsEnemy(Target);
}

void USnGA_SphereAttack::ExecuteSphereAttack(UAbilitySystemComponent* SourceASC, AActor* Avatar)
{
	UWorld* World = GetWorld();
	if (!SourceASC || !Avatar || !DamageEffectClass || !World)
	{
		return;
	}

	// How many targets took damage this swing; the swing cue at the end of
	// the body reads it (RawMagnitude) to decide whether to also play the
	// once-per-swing camera shake (in the notify asset).
	int32 NumDamaged = 0;

	// Damage = the attacker's damage of DamageType (base x (1 + percent)) x
	// Multiplier, growing per ability level - the shared damage-family formula
	// (USnGA_DamageSkill::ScaleDamageByLevel, same one the descriptions show).
	// One element per hit; the crit roll happens in USnExecCalc_Damage when
	// the GE executes. Defaults (Physical / 1.0 / no growth) reproduce the
	// basic attack.
	FSnDamageResult Damage = USnCombatLibrary::CalculateDamage(SourceASC, DamageType);
	Damage.Damage = USnGA_DamageSkill::ScaleDamageByLevel(Damage.Damage, Multiplier, PerLevelGrowth, GetAbilityLevel());

	// Strike where the player is looking: the aim is read off the controller as a
	// value (GetAvatarAimRotation) - the body faces its run direction, not the
	// camera. Only the role that asks for it turns the body onto that yaw
	// (ShouldFaceAimOnStrike: the input basic attack, whose swing should read as
	// hitting what the player aimed at); every other role leaves the facing alone,
	// so activating a skill never rotates the character. The sphere, the cues and
	// the knockback below all read this yaw, never the body's.
	const FRotator AimRotation = GetAvatarAimRotation(Avatar);
	const FVector AimDirection = AimRotation.Vector();
	if (ShouldFaceAimOnStrike())
	{
		Avatar->SetActorRotation(AimRotation);
	}

	// Detect targets in a sphere in front of the attacker (overlap, not trace:
	// traces misfire on simple-collision meshes). No class filter: the tag
	// team check below (USnCombatLibrary::IsEnemy) owns hostility, so future
	// enemy classes need no changes here.
	const FVector Center = Avatar->GetActorLocation() + AimDirection * SphereForwardDistance;
	TArray<AActor*> Overlapped;
	UKismetSystemLibrary::SphereOverlapActors(
		this,
		Center,
		SphereRadius,
		{ UEngineTypes::ConvertToObjectType(ECC_Pawn) },
		nullptr,
		{ Avatar },
		Overlapped);

	for (AActor* Target : Overlapped)
	{
		// Destructibles carry no ASC, so IsEnemy is never true for them:
		// route them to their own branch - one landed attack counts one hit
		// toward breaking (damage numbers never apply). Cue net rules are the
		// enemy hit cue's: the predicted client copy plays it locally and
		// absorbs the server's echo, so every side plays exactly one copy.
		if (ASnDestructible* Destructible = USnCombatLibrary::FindDestructible(Target))
		{
			Destructible->RegisterHit();
			NumDamaged++;
			if (HitCueTag.IsValid())
			{
				FGameplayCueParameters HitParams;
				HitParams.Location = Target->GetActorLocation();
				HitParams.Instigator = Avatar;
				SourceASC->ExecuteGameplayCue(HitCueTag, HitParams);
			}
			continue;
		}

		// Whom this swing damages is the role's decision (IsValidAttackTarget):
		// the Combat.Team.Enemy tag for every player-side skill (the same
		// hostility source as the projectiles and the homing), reversed by the
		// enemy attack.
		if (!IsValidAttackTarget(Target))
		{
			continue;
		}

		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (!TargetASC)
		{
			continue;
		}

		// Ship the damage spec (per-element magnitudes + penetration + crit
		// flag); the victim's AttributeSet resolves mitigation. Instigator =
		// the attacker, so kill credit keeps working. Zero-damage configs
		// (Multiplier 0) still swing and knock back, but skip the damage
		// pipeline - no hit event / feedback for a 0 hit.
		if (Damage.Damage > 0.f && USnCombatLibrary::ApplyDamageToTarget(SourceASC, DamageEffectClass, Avatar, TargetASC, Damage, GetAbilityLevel()))
		{
			// Hit event: equipment-granted enchantment GAs (AbilityTriggers on
			// Event.AttackHit) append their own independent damage application
			// per target, and bond passives (the ignite) filter by element.
			// Fired where the attack resolves, so the appended/ignite effects
			// go through the same ExecCalc pipeline. The hit's element rides on
			// the event's effect context (every project context is an
			// FSnGameplayEffectContext) - listeners read it with a plain cast,
			// no extra broadcast or side channel. The magnitude rides along as
			// the hit's damage, which is what the proc family's "share of the
			// triggering amount" payload reads (e.g. shield worth 20% of the
			// damage). It is the attacker-side number - what the swing was
			// worth BEFORE the victim's mitigation, the only one that exists on
			// this side of the pipeline (dodge / resistance / toughness /
			// defense resolve on the victim inside the application above, and
			// nothing comes back from it).
			FGameplayEventData EventData;
			EventData.Instigator = Avatar;
			EventData.Target = Target;
			EventData.EventMagnitude = Damage.Damage;
			FGameplayEffectContextHandle EventContext = SourceASC->MakeEffectContext();
			if (FSnGameplayEffectContext* SnEventContext = static_cast<FSnGameplayEffectContext*>(EventContext.Get()))
			{
				SnEventContext->SetDamageType(Damage.DamageType);
			}
			EventData.ContextHandle = EventContext;
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, SnGameplayTags::Event_AttackHit, EventData);

			// Hit stop (briefly freezing the target - the community-standard
			// combat juice; a miss never freezes anything). The target's
			// CustomTimeDilation is replicated actor state, so the freeze
			// reaches every client straight from here - no presentation
			// channel needed.
			ApplyHitStop(Target);

			// Hit cue at the target: the impact spark / sound live in the
			// notify asset (HitCueTag), spawned at the cue's Location (the
			// target). Same net rules as the swing cue below - the predicted
			// client copy plays it instantly and the server's echo of the
			// same prediction key is absorbed on the attacker, so every side
			// plays exactly one copy per damaged target.
			if (HitCueTag.IsValid())
			{
				FGameplayCueParameters HitParams;
				HitParams.Location = Target->GetActorLocation();
				// The attacker rides along in the cue params (replicated): the hit
				// notify may need instigator context (e.g. hit-stop / screen shake
				// decisions) without touching the effect context.
				HitParams.Instigator = Avatar;
				SourceASC->ExecuteGameplayCue(HitCueTag, HitParams);
			}

			NumDamaged++;
		}

		// Knockback: away from the attacker, slightly upward.
		if (ACharacter* TargetCharacter = Cast<ACharacter>(Target))
		{
			const FVector Direction = (Target->GetActorLocation() - Avatar->GetActorLocation()).GetSafeNormal2D();
			TargetCharacter->LaunchCharacter(Direction * LaunchForce + FVector::UpVector * LaunchUpForce, /*bXYOverride=*/false, /*bZOverride=*/false);
		}
	}

	// Swing cue: fired once per activation at the end of the shared body, hit
	// or miss - the arc VFX + whoosh live in the notify asset (SwingCueTag),
	// spawned at the cue's Location (50 cm in front of the attacker, facing
	// its Normal). RawMagnitude (1 = something took damage) gates the
	// notify's once-per-swing camera shake for the locally controlled
	// attacker. Net rules: the input attack's predicted client copy runs the
	// cue instantly with its prediction key and absorbs the server's echo of
	// the same key (keys only replicate to their originator - every other
	// client plays the echo), so each machine plays exactly one copy; the
	// one-shot's ServerOnly instance runs the cue once on the server and
	// multicasts it to all sides.
	if (SwingCueTag.IsValid())
	{
		FGameplayCueParameters SwingParams;
		SwingParams.Location = Avatar->GetActorLocation() + AimDirection * 50.f;
		SwingParams.Normal = AimDirection;
		SwingParams.RawMagnitude = NumDamaged > 0 ? 1.f : 0.f;
		// The attacker, so the swing notify can gate its once-per-swing camera
		// shake on IsLocallyControlled (observers never shake for someone
		// else's swing). NetSerialize carries Instigator (REP_Instigator), so
		// every side's copy of the cue sees it.
		SwingParams.Instigator = Avatar;
		SourceASC->ExecuteGameplayCue(SwingCueTag, SwingParams);
	}
}

void USnGA_SphereAttack::PlayAttackMontageAndWaitForWindow(UAnimMontage* InMontageToPlay, float InPlayRate, FName InStartSection)
{
	if (!InMontageToPlay)
	{
		return;
	}

	// Both tasks belong to this ability instance and die with it.
	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, InMontageToPlay, InPlayRate, InStartSection, /*bStopWhenAbilityEnds=*/true);
	UAbilityTask_WaitGameplayEvent* WindowTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this, SnGameplayTags::Event_AttackWindow);

	// Every exit from the montage ends the ability - EndAbility is guarded
	// against repeat calls (IsEndAbilityValid), so one handler serves all four
	// pins even though a normal finish fires OnBlendOut first and OnCompleted
	// later. A montage that fails to play (no AnimInstance, asset not on this
	// skeleton) broadcasts OnCancelled, so a misconfigured attack can never
	// wedge an InstancedPerActor instance and lock the owner out of swinging.
	MontageTask->OnCompleted.AddDynamic(this, &USnGA_SphereAttack::OnAttackMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &USnGA_SphereAttack::OnAttackMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &USnGA_SphereAttack::OnAttackMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &USnGA_SphereAttack::OnAttackMontageFinished);
	MontageTask->ReadyForActivation();

	// OnlyTriggerOnce stays false: the ability listens for the whole montage,
	// so every notify frame resolves (a multi-notify montage = a combo).
	WindowTask->EventReceived.AddDynamic(this, &USnGA_SphereAttack::OnAttackWindowEvent);
	WindowTask->ReadyForActivation();
}

void USnGA_SphereAttack::OnAttackWindowEvent(FGameplayEventData Payload)
{
	// Authority only - see the class doc: the notify fires on every machine
	// playing the montage (a predicted client copy included) and carries no
	// prediction key, so a client-side run would deal damage from an
	// unconfirmed state and double every gameplay cue against the server's own
	// multicast. Simulation proxies never even get here (they have no ability
	// instance running).
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->IsNetAuthority())
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = ActorInfo->AbilitySystemComponent.Get();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!SourceASC || !Avatar || !DamageEffectClass)
	{
		return;
	}

	// The full swing -> damage -> hit feedback flow, gameplay cues included,
	// lives in the shared core - the authoritative copy runs it once and the
	// core's cues multicast to every client (see ExecuteSphereAttack).
	ExecuteSphereAttack(SourceASC, Avatar);
}

void USnGA_SphereAttack::OnAttackMontageFinished()
{
	// bReplicateEndAbility=false on purpose: a client-side end RPC would reach
	// the server and kill its copy mid-montage, before its own notify frame
	// ever fired - losing the hit. Each side ends its own copy when its own
	// animation is done.
	EndAbility(GetCurrentAbilitySpecHandle(), GetCurrentActorInfo(), GetCurrentActivationInfo(),
		/*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
}

void USnGA_SphereAttack::ApplyHitStop(AActor* Target)
{
	if (!Target || HitStopTimeDilation <= 0.f)
	{
		return;
	}

	// AActor has no setter for this (SetCustomTimeDilation lives on ACharacter)
	// - the property is public and directly assignable on any actor.
	Target->CustomTimeDilation = HitStopTimeDilation;

	// Per-target restore timer, bound to the target itself via a weak lambda:
	// as long as the target lives the restore always fires, and if it dies
	// mid-stop (DOT, another attack) the timer dies with it. Binding the GA
	// instead silently loses the timer - blueprint abilities are instanced per
	// execution and the instance is gone right after EndAbility, which left
	// targets frozen at 0.05x (barely moving, floating in the knockback arc).
	// Plain SetTimer is fine here: it counts game time, which only scales with
	// GLOBAL dilation - CustomTimeDilation never stretches it. (5.7 removed
	// SetTimerRealTime; the community recipe's real-time restore therefore has
	// no API left, and none is needed while only target dilation is used.)
	FTimerHandle RestoreHandle;
	FTimerDelegate RestoreDelegate = FTimerDelegate::CreateWeakLambda(Target, [Target]()
	{
		Target->CustomTimeDilation = 1.f;
	});
	GetWorld()->GetTimerManager().SetTimer(RestoreHandle, RestoreDelegate, HitStopDuration, /*bLoop=*/false);
}

FText USnGA_SphereAttack::GetDescription_Implementation(int32 Level)
{
	// The shared damage-family element clause (same growth expression the
	// damage formula uses) wrapped in the sphere family's own sentence.
	return FText::Format(
		FText::FromString(TEXT("对前方范围内的敌人造成 {0}，并将其击退")),
		USnGA_DamageSkill::BuildDamageClause(Level, Multiplier, PerLevelGrowth, DamageType));
}
