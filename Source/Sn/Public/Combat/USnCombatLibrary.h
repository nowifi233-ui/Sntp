// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
// 5.7's GAS lives in a plugin and GameplayEffectSpec.h was split up -
// FGameplayEffectSpecHandle is defined here (USTRUCT(BlueprintType), so the
// UFUNCTION below needs the full type, not a forward declaration).
#include "GameplayEffectTypes.h"
// UGameplayEffect itself lives in GameplayEffect.h (GameplayEffectTypes.h does
// not define it). The full type is required: UHT only emits a pickable class
// pin for TSubclassOf<T> when T is defined - a forward declaration degrades
// the pin to a plain UClass* object pin that can only be wired, not picked.
#include "GameplayEffect.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "USnCombatLibrary.generated.h"

class UAbilitySystemComponent;
struct FGameplayTag;
class ASnDestructible;

/** Damage categories. A hit carries exactly ONE element - each damage
 *  application deals a single element. Extra damage (e.g. a skill dealing
 *  100% physical + 20% lightning) is a second, independent damage
 *  application, not a multi-element spec. The victim's element resistance
 *  applies inside the damage ExecCalc (USnExecCalc_Damage). */
UENUM(BlueprintType)
enum class EDamageType : uint8
{
	Physical,
	Fire,
	Ice,
	Lightning
};

/** A single damage application: the element and its pre-mitigation magnitude
 *  (base[type] x (1 + percent[type]); NO crit - the attacker's crit roll moved
 *  into USnExecCalc_Damage). Mitigation (dodge / resistance / toughness /
 *  defense) happens when the damage GE executes. */
USTRUCT(BlueprintType)
struct FSnDamageResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	EDamageType DamageType = EDamageType::Physical;

	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	float Damage = 0.f;

	/** This application answers a hit rather than being one: set by the
	 *  counter-attack passives (USnGA_EventProc's damage payload on
	 *  Event.DamageTaken) so the flag rides the effect context and the victim's
	 *  own counter-attack passive knows not to answer it in turn. Everything
	 *  else about the hit is ordinary - mitigation, floating number, kill
	 *  credit, and the victim's Event.DamageTaken all still happen. See
	 *  FSnGameplayEffectContext::SetIsReflected. */
	UPROPERTY(BlueprintReadWrite, Category = "Damage")
	bool bReflected = false;
};

/**
 * Combat relationship helpers. "Who is an enemy" is decided by a GameplayTag
 * on the target's ASC (Combat.Team.Enemy), not by class: any pawn carrying the
 * tag is hostile, anything else (player, companions, neutrals) is not. New
 * enemy types only need to add the tag - no code changes.
 *
 * Target queries live here too, so every consumer (companion AI, GA
 * blueprints) shares one overlap + filter + sort implementation instead of
 * re-implementing it.
 *
 * Damage application lives here as well: attackers compute their ONE element
 * (base[type] x (1 + percent[type])) and ship it via ApplyDamageToTarget as a
 * single SetByCaller spec with the element flag on the effect context. The
 * victim resolves mitigation (dodge / resistance / crit / toughness /
 * defense) in USnExecCalc_Damage when the effect executes; the attribute set
 * only applies the result to Health. Both the player attack GA and the enemy
 * touch damage use this one path.
 */
UCLASS()
class SN_API USnCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** True if the actor's ASC carries the enemy team tag. Actors without an
	 *  ASC (or without the tag) are never enemies. */
	UFUNCTION(BlueprintPure, Category = "Sn|Combat")
	static bool IsEnemy(AActor* InActor);

	/** The destructible on an attack target, or null. Destructibles carry no
	 *  ASC (IsEnemy is always false for them): the attack cores branch on this
	 *  BEFORE the IsEnemy test and count a hit toward breaking - the lookup
	 *  keeps that branch out of the ability code. */
	static ASnDestructible* FindDestructible(AActor* InActor);

	/** Nearest enemy to Center within Radius, or null. The companion AI
	 *  controller and ability blueprints all call this one query. */
	UFUNCTION(BlueprintPure, Category = "Sn|Combat", meta = (WorldContext = "WorldContextObject"))
	static AActor* FindNearestEnemy(UObject* WorldContextObject, FVector Center, float Radius);

	/** Computes the attacker-side damage for ONE element: base[type] x (1 +
	 *  percent[type]). No crit roll here - the attacker's crit moved into
	 *  USnExecCalc_Damage (it captures CritChance/CritDamage on the source).
	 *  Pre-mitigation result; the victim's mitigation applies when the damage
	 *  GE executes. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Combat")
	static FSnDamageResult CalculateDamage(UAbilitySystemComponent* SourceASC, EDamageType DamageType);

	/** Builds a single-element damage spec WITHOUT applying it - for delayed
	 *  delivery, e.g. a projectile that applies on hit (the shoot ability hands
	 *  the spec to the projectile). Same spec ApplyDamageToTarget applies
	 *  immediately: outgoing GE + negative SetByCaller under the element's key
	 *  + the element flag and instigator on the effect context. All of it set
	 *  here in C++ - a blueprint cannot reach the context's element flag
	 *  (SetDamageType is native-only), and a missing instigator silently kills
	 *  projectile delivery (the hit lookup). The damage GE must carry the
	 *  USnExecCalc_Damage execution, which resolves the mitigation. AbilityLevel
	 *  scales the GE (scalable floats); 1 when unused. bReflected marks the spec
	 *  as an answer to a hit (see FSnDamageResult::bReflected) - it rides the
	 *  effect context, so it must be set HERE, with the element flag. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Combat")
	static FGameplayEffectSpecHandle MakeDamageSpec(UAbilitySystemComponent* SourceASC, TSubclassOf<UGameplayEffect> DamageEffectClass, AActor* Instigator, float Damage, EDamageType DamageType, float AbilityLevel = 1.f, bool bReflected = false);

	/** Ships Damage to TargetASC as a single-element damage spec: makes the
	 *  outgoing GE, writes the element's SetByCaller magnitude (negative) +
	 *  the element flag onto the effect context, and applies it. The damage GE
	 *  must carry the USnExecCalc_Damage execution, which resolves the
	 *  mitigation. Instigator = the attacking actor (kill credit, effect
	 *  context). AbilityLevel scales the GE (scalable floats); 1 when unused.
	 *  Returns whether the spec was built and applied (callers gate their
	 *  hit feedback on this). */
	UFUNCTION(BlueprintCallable, Category = "Sn|Combat")
	static bool ApplyDamageToTarget(UAbilitySystemComponent* SourceASC, TSubclassOf<UGameplayEffect> DamageEffectClass, AActor* Instigator, UAbilitySystemComponent* TargetASC, const FSnDamageResult& Damage, float AbilityLevel = 1.f);

	/** SetByCaller key under which the damage GE carries the element's
	 *  magnitude (SetByCaller.Damage.<Element>). Shared by ApplyDamageToTarget
	 *  (write) and USnExecCalc_Damage (read). */
	UFUNCTION(BlueprintPure, Category = "Sn|Combat")
	static FGameplayTag GetDamageSetByCallerKey(EDamageType DamageType);

	/** Rich-text style tag naming the damage number's style row (the shared
	 *  TextStyleSet DataTable used by WBP_DamageNumber): "Damage.Physical" /
	 *  "Damage.Fire" / "Damage.Ice" / "Damage.Lightning". Physical is the
	 *  default - any element that has no dedicated row falls back to it. */
	UFUNCTION(BlueprintPure, Category = "Sn|Combat")
	static FName GetDamageNumberTag(EDamageType DamageType);

	/** Display name of an element ("物理" / "火焰" / "冰霜" / "闪电"), shared by
	 *  the ability descriptions. */
	UFUNCTION(BlueprintPure, Category = "Sn|Combat")
	static FText GetDamageTypeDisplayName(EDamageType DamageType);

	/** Rich-text style tag naming the element's text row in the shared
	 *  DT_TextStyles table ("Element.Physical" / ".Fire" / ".Ice" /
	 *  ".Lightning" - the ability-description counterpart of the damage
	 *  number's Damage.* tags). A missing row falls back to the default
	 *  style, so an unconfigured element still renders. */
	UFUNCTION(BlueprintPure, Category = "Sn|Combat")
	static FName GetDamageTypeStyleTag(EDamageType DamageType);

	/** All actors within Radius whose ASC carries the enemy team tag. AoE
	 *  consumers (e.g. the frost nova release) damage the whole list;
	 *  FindNearestEnemy uses the same query for the single best pick. */
	static TArray<AActor*> GetEnemiesInRadius(UObject* WorldContextObject, FVector Center, float Radius);
};
