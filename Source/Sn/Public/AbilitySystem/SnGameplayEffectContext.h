// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"
#include "Combat/USnCombatLibrary.h"
#include "SnGameplayEffectContext.generated.h"

/**
 * Effect-context extension carrying the hit's combat flags. Every effect
 * context created in this project is this type (USnAbilitySystemGlobals
 * allocates it, see Sn.cpp / DefaultGame.ini), so the damage ExecCalc and the
 * attribute set exchange the element and the resolved crit/miss flags without
 * extra SetByCaller side channels - and the flags replicate with the context.
 */
USTRUCT()
struct SN_API FSnGameplayEffectContext : public FGameplayEffectContext
{
	GENERATED_BODY()

public:
	FSnGameplayEffectContext() = default;

	/** The element this hit carries (single-element model: extra damage of
	 *  another element is a second, independent damage application). */
	void SetDamageType(EDamageType InDamageType) { DamageType = InDamageType; }
	EDamageType GetDamageType() const { return DamageType; }

	/** Resolution flags - written by USnExecCalc_Damage while the damage GE
	 *  executes, read by USnAttributeSet when it broadcasts the hit result. */
	void SetIsCriticalHit(bool bInCriticalHit) { bIsCriticalHit = bInCriticalHit; }
	bool IsCriticalHit() const { return bIsCriticalHit; }

	void SetIsMissed(bool bInMissed) { bIsMissed = bInMissed; }
	bool IsMissed() const { return bIsMissed; }

	/** True when this damage application is a riposte: one raised by a passive
	 *  answering Event.DamageTaken (USnGA_EventProc's damage payload, i.e. the
	 *  thorns / counter-attack case). The damage itself is ordinary - full
	 *  ExecCalc pipeline, floating number, kill credit - and the victim's
	 *  Event.DamageTaken is raised as usual. The flag only tells a listener
	 *  "do not answer this with another riposte": without it two units wearing
	 *  the same passive would bounce damage back and forth until the stack
	 *  dies (the proc's re-entrancy latch is per INSTANCE, so it cannot see the
	 *  other unit's copy). With it the chain stops at depth 2. */
	void SetIsReflected(bool bInReflected) { bIsReflected = bInReflected; }
	bool IsReflected() const { return bIsReflected; }

	/** Required so the context can be serialized / duplicated (replication,
	 *  prediction, effect instantiation all touch these). */
	virtual UScriptStruct* GetScriptStruct() const override;
	virtual FGameplayEffectContext* Duplicate() const override;
	virtual bool NetSerialize(FArchive& Ar, UPackageMap* Map, bool& bOutSuccess) override;

private:
	EDamageType DamageType = EDamageType::Physical;

	/** The attacker's crit roll passed (pre-mitigation roll; the damage still
	 *  goes through dodge / resistances / toughness / defense). */
	bool bIsCriticalHit = false;

	/** The victim's dodge roll passed - the hit dealt 0. */
	bool bIsMissed = false;

	/** This damage answers a hit (see SetIsReflected). */
	bool bIsReflected = false;
};

/** Required for network replication: without the trait specialization the
 *  struct's UScriptStruct never gets STRUCT_NetSerializeNative, and the
 *  engine Fatal-logs in FGameplayEffectContextHandle::NetSerialize when any
 *  active effect carrying this context replicates (first seen on LAN: the CD
 *  GE applied by a basic attack). Same pattern as the engine's base
 *  FGameplayEffectContext (GameplayEffectTypes.h) and Aura. */
template<>
struct TStructOpsTypeTraits<FSnGameplayEffectContext> : public TStructOpsTypeTraitsBase2<FSnGameplayEffectContext>
{
	enum
	{
		WithNetSerializer = true,
	};
};

/**
 * Global GAS override (registered in DefaultGame.ini under
 * AbilitySystemGlobalsClassName): every ASC context comes out as an
 * FSnGameplayEffectContext, so the flags above are always available - on the
 * server, replicated to clients, and through prediction.
 */
UCLASS()
class SN_API USnAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()

public:
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
};
