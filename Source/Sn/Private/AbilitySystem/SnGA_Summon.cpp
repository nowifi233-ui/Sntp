// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_Summon.h"

#include "AbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"

USnGA_Summon::USnGA_Summon()
{
	// Spawning (and the EndAbility cleanup) must be server-authoritative.
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// The equip flow grants AND activates this (bActivateOnEquip is read by
	// USnItemLibrary::GrantEquipAbilities right after the grant). No
	// OnAvatarSet self-activation: the equip flow owns activation.
	bActivateOnEquip = true;
}

void USnGA_Summon::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !SummonClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("USnGA_Summon %s: SummonClass or avatar missing"), *GetName());
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Same pattern as the attack GA: activation is gated by CanActivateAbility,
	// so a commit failure here means something invalidated the ability
	// mid-activation - bail out rather than summoning.
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	FVector Location;
	if (!FindSpawnLocation(Avatar->GetActorLocation(), Location))
	{
		UE_LOG(LogTemp, Warning, TEXT("USnGA_Summon %s: no ground under any candidate, spawning at the caster"), *GetName());
	}

	// This GA only manages its own summon: spawn it, set the caster as the
	// owner (the companion family binds the owner's death through it), and
	// watch it - its death ends this activation (see HandleSummonDestroyed).
	// Other summons in the world are none of this activation's business.
	AActor* SpawnedSummon = GetWorld()->SpawnActor<AActor>(SummonClass, Location, FRotator::ZeroRotator);
	if (SpawnedSummon)
	{
		SpawnedSummon->SetOwner(Avatar);
		OwnedSummons.Add(SpawnedSummon);
		SpawnedSummon->OnDestroyed.AddDynamic(this, &USnGA_Summon::HandleSummonDestroyed);
	}

	// Stay active on purpose: the ability's active window IS the summon's
	// lifetime. Unequip clears the spec (the engine ends the active instance),
	// cancel routes into EndAbility - both destroy the summon below.
}

void USnGA_Summon::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// The single cleanup point for every way the ability can end (UE5 routes
	// cancel into EndAbility, and removing the spec ends an active instance -
	// both verified in 5.7 engine source). Only OUR summons die here.
	//
	// Swap the list out BEFORE destroying: every Destroy re-fires
	// HandleSummonDestroyed, which must not see its own cleanup as a
	// combat death and re-enter EndAbility.
	TArray<TWeakObjectPtr<AActor>> Summons;
	Swap(Summons, OwnedSummons);
	for (const TWeakObjectPtr<AActor>& Summon : Summons)
	{
		if (AActor* SummonActor = Summon.Get())
		{
			SummonActor->Destroy();
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool USnGA_Summon::FindSpawnLocation(const FVector& Origin, FVector& OutLocation) const
{
	// Spawn height = ground + capsule half height, read from the summon class
	// CDO so capsule tweaks in the BP stay aligned automatically (same idea as
	// the survival spawner's SnapToGround, plus the 5 cm resting margin). A
	// non-character summon class has no capsule - fall back to the 88 cm
	// standing height.
	float HalfHeight = 88.f;
	if (SummonClass)
	{
		if (const ACharacter* SummonCDO = Cast<ACharacter>(SummonClass->GetDefaultObject()))
		{
			HalfHeight = SummonCDO->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		}
	}

	// Random circle point around the caster, each probed straight down (same
	// 20 m vertical window as the survival spawner). A miss (candidate over a
	// cliff edge) skips the point - never spawn in the void.
	for (int32 Attempt = 0; Attempt < 8; ++Attempt)
	{
		const float Angle = FMath::FRandRange(0.f, 2.f * PI);
		FVector Candidate = Origin + FVector(FMath::Cos(Angle), FMath::Sin(Angle), 0.f) * SpawnRadius;

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(
				Hit,
				Candidate + FVector(0.f, 0.f, 2000.f),
				Candidate - FVector(0.f, 0.f, 2000.f),
				ECC_WorldStatic))
		{
			OutLocation = Candidate;
			OutLocation.Z = Hit.ImpactPoint.Z + HalfHeight + 5.f;
			return true;
		}
	}

	// Every candidate missed (the caster stands next to a void): fall back to
	// the caster's own location - the caster stands on ground by definition.
	OutLocation = Origin;
	return false;
}

void USnGA_Summon::HandleSummonDestroyed(AActor* DestroyedActor)
{
	// Not one of ours anymore: either it was never managed, or EndAbility is
	// destroying it right now (the list was swapped out before the destroy
	// loop) - stay out of the re-entrancy.
	int32 Index = OwnedSummons.IndexOfByPredicate([DestroyedActor](const TWeakObjectPtr<AActor>& Summon)
	{
		return Summon.Get() == DestroyedActor;
	});
	if (Index == INDEX_NONE)
	{
		return;
	}
	OwnedSummons.RemoveAt(Index);

	// A managed summon died in combat: the active window ends with it. The
	// ended ability lets a sustain trigger re-release the summon on its next
	// pulse (the summon-stream combo, see the class doc).
	if (IsActive())
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
	}
}
