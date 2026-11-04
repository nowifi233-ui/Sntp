// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGameplayAbility.h"
#include "SnGA_Summon.generated.h"

class AActor;

/**
 * Summon ability family core (Abstract): spawns a configured actor class
 * (SummonClass) next to the caster, sets the caster as its owner and keeps it
 * alive while the ability is active. The ability's active window IS the
 * summon's lifetime - EndAbility (cancel, or spec removal: unequipping the
 * item clears the spec and the engine ends the active instance) destroys
 * THIS activation's summon; and a managed summon dying in combat ends the
 * ability, so the window closes with it.
 *
 * That death-ends-the-ability rule is what enables the summon-stream combo: a
 * sustain trigger releasing summon abilities re-releases once its previous
 * summon is gone (the ended ability lets the next pulse's TryActivateAbility
 * through). Single-summon family semantics: each activation manages its own
 * summon and the ability ends with it - a future multi-summon shell must end
 * only when its LAST summon dies (override HandleSummonDestroyed's rule).
 *
 * The equip flow grants AND activates this in one step (bActivateOnEquip, read
 * by USnItemLibrary::GrantEquipAbilities) - equipping - and restoring after a
 * level travel - summon without the ability hooking engine events. ServerOnly:
 * spawn and cleanup are authoritative, like the engine one-shot / attack
 * families.
 */
UCLASS(Abstract)
class SN_API USnGA_Summon : public USnGameplayAbility
{
	GENERATED_BODY()

public:
	USnGA_Summon();

	/** Class to spawn (e.g. BP_Companion). The summon's combat strength is its
	 *  own (its blueprint / data table) - this ability only configures the
	 *  class and the spawn point (numbers-in-one-place rule). */
	UPROPERTY(EditDefaultsOnly, Category = "Summon")
	TSubclassOf<AActor> SummonClass;

	/** Spawn scatter radius around the caster. */
	UPROPERTY(EditDefaultsOnly, Category = "Summon", meta = (ClampMin = "0.0"))
	float SpawnRadius = 100.f;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Destroys every summon this activation manages, then ends. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** Picks a random circle point around Origin, snapped onto the ground under
	 *  its own X/Y (spawn height = ground + capsule half height from the
	 *  summon CDO). Falls back to Origin when every candidate misses. */
	bool FindSpawnLocation(const FVector& Origin, FVector& OutLocation) const;

	/** A managed summon was destroyed: drop it from the bookkeeping, and if it
	 *  died on its own (not from this ability's EndAbility - the list is
	 *  already cleared there) the active window ends with it. */
	UFUNCTION()
	void HandleSummonDestroyed(AActor* DestroyedActor);

	/** The summons this activation spawned; EndAbility destroys them. Weak
	 *  pointers: a summon dying in combat is dropped by HandleSummonDestroyed
	 *  instead of dangling here. */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> OwnedSummons;
};
