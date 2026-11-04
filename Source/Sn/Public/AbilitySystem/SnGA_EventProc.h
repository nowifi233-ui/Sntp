// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_DamageSkill.h"
#include "AttributeSet.h"
#include "Combat/USnCombatLibrary.h"
#include "GameplayTagContainer.h"
#include "SnGA_EventProc.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;
struct FGameplayEventData;
struct FSnGameplayEffectContext;

/**
 * Where a proc's payload lands. Besides the listener, an event carries two
 * actors - Payload.Instigator and Payload.Target - and their MEANING is defined
 * by the event, not by this enum: on Event.DamageTaken the instigator is the
 * attacker, on Event.AttackHit the target is the victim (see the class doc's
 * table). An ability payload ignores this setting (an ability only ever
 * activates on its own ASC).
 */
UENUM(BlueprintType)
enum class ESnEventTargetMode : uint8
{
	/** The listener's own ASC - the default. */
	Self,

	/** Payload.Instigator: who raised the event. The attacker on
	 *  Event.DamageTaken (the counter-attack case), the healer on a heal. */
	EventInstigator,

	/** Payload.Target: who the event is about - the victim on
	 *  Event.AttackHit (the on-hit append case). */
	EventTarget
};

/**
 * Event-proc family core: an equipment passive configured along three
 * orthogonal dimensions - WHAT to listen for, WHO the payload lands on, and
 * WHAT the payload does. A proc IS a configured instance of this class
 * (NOT Abstract: every dimension is data, so there is nothing for a shell to
 * implement); blueprint subclasses carry the values, the C++ class stays one.
 *
 *  1. Trigger - TriggerEventTags (which events to listen to) +
 *     EventsPerTrigger (the "每 N 次" counter) + TriggerDamageTypes (element
 *     filter on the damage events).
 *  2. Target - PayloadTargetMode: Self / EventInstigator / EventTarget. The two
 *     event actors, per event:
 *
 *        event                | Instigator        | Target
 *        ---------------------|-------------------|------------------
 *        Event.AttackHit      | the listener      | the victim
 *        Event.DamageTaken    | the attacker      | the listener
 *        Event.AttributeGained| the effect source | the listener
 *        Event.Kill           | the listener      | the listener
 *
 *     The resolved actor must carry an ASC (both payloads below need one) -
 *     actors without one (destructibles) are skipped silently. There is
 *     deliberately NO spatial targeting (nearest enemy, enemies in radius,
 *     the event's location): that is the sphere / projectile families'
 *     business, reachable through the ability payload below.
 *  3. Payload - two effect slots and one ability slot, all optional and
 *     combining freely. Each slot has its own enable switch (bEnable*Payload)
 *     that BOTH hides its fields in the details panel (a fresh asset reads as
 *     three switches, one per slot) AND gates execution - a slot left off never
 *     fires, whatever its fields still hold (the values are kept, so ticking
 *     the box back on restores the old configuration):
 *       A. the effect payload (Effect*): a SetByCaller stat modifier applied
 *          through USnItemLibrary::ApplyItemModifiers - exactly like an equip
 *          row, so the numbers live on the shared generic effects.
 *       B. the damage payload (Damage*): one element through the standard
 *          damage pipeline (USnCombatLibrary::ApplyDamageToTarget - ExecCalc
 *          mitigation, floating number, kill credit), flat value + a share of
 *          the triggering amount.
 *       C. the ability payload (TriggeredAbilityClass): pre-granted for the
 *          whole active window and released with a plain TryActivateAbility
 *          (the timed-trigger pattern). An ability always runs on its own ASC,
 *          so this slot ignores PayloadTargetMode and is the ONLY slot that
 *          can do something spatial (its own shape picks the targets).
 *
 * The two payloads grow with the effective item level (item level + rune cell
 * bonus), each half by its own convention: the flat values as
 * Value + Level x ValuePerLevel (the equip-row convention, applied by the item
 * path for the effect payload and computed here for the damage payload), the
 * shares as Multiplier + Level x PerLevelGrowth through
 * USnGA_DamageSkill::ScaleDamageByLevel (the damage-family convention). The
 * two never multiply each other.
 *
 * Which events exist (all raised by the project itself, all documented on their
 * tag in SnGameplayTags.h):
 *  - Event.AttackHit - fired per hit TARGET by the damage families, from the
 *    attacker's side; magnitude = the hit's damage, PRE-mitigation.
 *  - Event.DamageTaken - fired at the VICTIM when a hit lands; magnitude = the
 *    resolved damage, the number the floating text shows.
 *  - Event.AttributeGained.Health / .Shield / .MP - fired by USnAttributeSet
 *    when an effect actually RAISED that attribute; magnitude = the applied
 *    delta.
 *  - Event.Kill - fired at the KILLER's ASC when something they killed dies;
 *    magnitude = 0 (a kill has no number to scale by, so a damage payload on it
 *    needs a flat value - and with both its actors being the listener, a damage
 *    payload on Event.Kill has nowhere to land anyway; use the ability slot for
 *    "on kill, do X").
 * One ratio field per payload serves every trigger kind: the payload reads the
 * event's magnitude without caring which event raised it, so "20% of the damage
 * dealt", "20% of the damage taken" and "20% of the amount healed" are the same
 * expression. Naming several tags in TriggerEventTags pools them into one count
 * ("hit OR healed").
 *
 * Element filter: TriggerDamageTypes narrows the two damage events by the hit's
 * element. It is skipped for the other events (their context carries no real
 * element), and a non-matching hit does not advance the counter either. The
 * damage payload's own bUseIncomingElement has the same rule for the same
 * reason.
 *
 * Equip-flow passive (bActivateOnEquip): the item-equip grant path activates it
 * right after granting and it stays alive for the whole equip window, so a proc
 * never pays per-trigger activation cost. Unequipping revokes the ability,
 * which cancels this activation and unbinds the listener (EndAbility).
 *
 * Server-authoritative (ServerOnly + InstancedPerActor): every trigger source
 * only fires where the effect executes, the counter is instance state that is
 * deliberately NOT replicated (a client-side copy would count a different
 * subset of events and drift), and the payload's own effects / cues replicate
 * by themselves.
 *
 * Termination: two loops have to be closed, and they are closed in different
 * places.
 *  - Same instance: payload execution is guarded by a re-entrancy latch. The
 *    handler runs synchronously INSIDE the effect application that raised the
 *    event, and a payload of ours applies another effect, which raises another
 *    event - a proc whose payload feeds its own trigger (heal -> shield on a
 *    shield listener) would otherwise spin forever. A trigger that arrives
 *    while this proc is paying out is dropped; sequential triggers are
 *    untouched. Design payloads so they do not feed their own trigger.
 *  - Across instances: the latch cannot help, because a second unit wearing the
 *    same passive has its own latch. A damage payload answering
 *    Event.DamageTaken is therefore marked as a RIPOSTE on the effect context
 *    (FSnDamageResult::bReflected -> FSnGameplayEffectContext::SetIsReflected),
 *    and a riposte never raises another one: two units with the same thorns
 *    passive trade exactly one blow each instead of bouncing damage back and
 *    forth until the stack dies. Only the damage payload is gated - the effect
 *    payload still pays out on a riposte hit, because taking one is still
 *    taking a hit ("gain shield when hit" must fire). Damage raised by any
 *    OTHER event is NOT marked: the victim of an enchanted hit must still be
 *    able to retaliate.
 *
 * Shield note: a shield payload must be an Instant effect - shield grants MUST
 * be Instant + AddBase (see USnAttributeSet; anything else registers an
 * aggregator modifier and the pool GROWS each time it absorbs a hit). The
 * shared GE_Generic_Use is Instant and already carries the SetByCaller.Shield
 * row.
 */
UCLASS()
class SN_API USnGA_EventProc : public USnGA_DamageSkill
{
	GENERATED_BODY()

public:
	USnGA_EventProc();

	// --- 1. Trigger ---

	/** Events this proc counts. Any tag in the container feeds the same counter
	 *  (the listener is a tag-container delegate, so a container with several
	 *  tags counts all of them together). Defaults to Event.AttackHit - the
	 *  common "on hit" proc - and a heal / shield listener swaps it for the
	 *  matching Event.AttributeGained.* tag. Empty = the ability ends itself on
	 *  activation (misconfigured asset, logged as a warning). */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger")
	FGameplayTagContainer TriggerEventTags;

	/** Triggers needed per payout: 1 = every event, 3 = every third one (the
	 *  counter resets after each payout and is per-instance, so it survives
	 *  across targets and never resets on its own). */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger", meta = (ClampMin = "1"))
	int32 EventsPerTrigger = 1;

	/** Element filter on the damage events: EMPTY = every element. Only
	 *  Event.AttackHit and Event.DamageTaken carry a real element (every other
	 *  event's context holds a meaningless default), so those two are the only
	 *  ones this narrows - a heal trigger is unaffected by it. A hit of another
	 *  element is rejected before the counter, so "every 3 fire hits" is never
	 *  advanced by physical ones.
	 *
	 *  Which elements count is the HIT's element, i.e. the damage type its spec
	 *  was built with: a fire skill's swing, its projectile and a fire-flavoured
	 *  hit all qualify. Secondary damage (an enchantment's appended element, a
	 *  burn tick) is not an event at all - see Event.AttackHit's tag doc.
	 *
	 *  List several entries to cover a multi-element passive ({Fire, Lightning}),
	 *  or the three elements for "any elemental damage, physical excluded". */
	UPROPERTY(EditDefaultsOnly, Category = "Trigger")
	TArray<EDamageType> TriggerDamageTypes;

	// --- 2. Target ---

	/** Where the two payloads land (the ability payload always runs on the
	 *  listener). See ESnEventTargetMode - and the class doc's table for what
	 *  each event's instigator / target actually is. The resolved actor needs an
	 *  ASC; without one the payload is skipped. */
	UPROPERTY(EditDefaultsOnly, Category = "Target")
	ESnEventTargetMode PayloadTargetMode = ESnEventTargetMode::Self;

	// --- 3a. Payload: the effect (a stat modifier) ---

	/** Tick to configure this payload: the whole block below stays hidden until
	 *  it is on, so a fresh asset reads as three switches and nothing else.
	 *
	 *  The box owns its own ROW on purpose - a hidden row takes its inline
	 *  checkbox with it, so gating the row that carries the switch would make
	 *  the payload impossible to switch back on (PropertyNode.cpp
	 *  SupportsEditConditionToggle builds the box inside the conditioned row,
	 *  SPropertyEditorTableRow collapses rows owned by EditConditionHides).
	 *  Off by default, and every block field carries
	 *  EditCondition = this + EditConditionHides.
	 *
	 *  Runtime mirrors it: a disabled payload never fires no matter what its
	 *  fields hold (the values stay in the asset, so re-ticking restores them) -
	 *  ActivateAbility warns once when a configured payload is left disabled. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (DisplayName = "启用属性载荷"))
	bool bEnableEffectPayload = false;

	/** Attribute the effect payload modifies (Shield / Health / PhysicalDamage /
	 *  MoveSpeed / ...). Resolved against the shared generic effects by
	 *  set-by-caller key, exactly like an equip row - the effect asset must
	 *  carry that attribute's SetByCaller row. Invalid (or with no EffectClass)
	 *  = this payload is off. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (EditCondition = "bEnableEffectPayload", EditConditionHides))
	FGameplayAttribute EffectAttribute;

	/** Effect applied on payout. Instant for a resource grant (shield / heal /
	 *  XP), Has Duration for a buff or a debuff (the shared GE_Generic_Use /
	 *  GE_Generic_Buff pair). */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (EditCondition = "bEnableEffectPayload", EditConditionHides))
	TSubclassOf<UGameplayEffect> EffectClass;

	/** Flat amount granted at ability level 0 (the base item level). */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "0.0", EditCondition = "bEnableEffectPayload", EditConditionHides))
	float EffectValue = 0.f;

	/** Flat amount added per ability level - the equip-row convention
	 *  (EffectiveValue = Value + Level * ValuePerLevel, levels start at 0). */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "0.0", EditCondition = "bEnableEffectPayload", EditConditionHides))
	float EffectValuePerLevel = 0.f;

	/** Share of the triggering event's magnitude added on top: 0.2 = 20% of the
	 *  damage dealt (Event.AttackHit), of the damage taken (Event.DamageTaken)
	 *  or of the amount healed / shield gained (Event.AttributeGained.*). One
	 *  field for every trigger kind - the payload never needs to know which
	 *  event fired. 0 = pure flat value. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "0.0", EditCondition = "bEnableEffectPayload", EditConditionHides))
	float EffectMagnitudeRatio = 0.f;

	/** Extra share per ability level, on top of EffectMagnitudeRatio - the same
	 *  growth convention as EffectValuePerLevel, and both are evaluated at the
	 *  EFFECTIVE item level (item level + rune cell bonus):
	 *      Ratio = EffectMagnitudeRatio + Level x EffectMagnitudeRatioPerLevel
	 *  "heal 20% of the damage dealt, +2% per level" is one row with 0.2 / 0.02.
	 *  The flat value and the share grow independently; leave either at 0 to drop
	 *  that half. Only the share's own growth lives here - the flat value's
	 *  per-level term is applied by the item path further down, so the two never
	 *  multiply each other. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "0.0", EditCondition = "bEnableEffectPayload", EditConditionHides))
	float EffectMagnitudeRatioPerLevel = 0.f;

	/** Buff length in seconds, overriding the effect asset's duration (only
	 *  meaningful for a Has Duration effect; requires an EffectClass that has
	 *  one). 0 = keep the asset's own duration. */
	UPROPERTY(EditDefaultsOnly, Category = "Effect", meta = (ClampMin = "0.0", EditCondition = "bEnableEffectPayload", EditConditionHides))
	float EffectDurationSeconds = 0.f;

	// --- 3b. Payload: damage ---

	/** Tick to configure this payload - same switch pattern as the effect
	 *  payload above (own row, everything else EditConditionHides). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (DisplayName = "启用伤害载荷"))
	bool bEnableDamagePayload = false;

	/** Damage effect carrying USnExecCalc_Damage (e.g. GE_AttackDamage) - the
	 *  very asset the attack skills use. Null = this payload is off. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (EditCondition = "bEnableDamagePayload", EditConditionHides))
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** Element of the damage payload when it does not follow the incoming one.
	 *  Decides the victim's resistance, the ExecCalc's SetByCaller key and the
	 *  floating number's colour. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (EditCondition = "bEnableDamagePayload", EditConditionHides))
	EDamageType DamageType = EDamageType::Physical;

	/** Take the element from the hit that raised the event instead of
	 *  DamageType - "whatever you hit me with, I hit you back with". Only the
	 *  two damage events carry a real element, so on the other events (and
	 *  without a context) this falls back to DamageType. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (EditCondition = "bEnableDamagePayload", EditConditionHides))
	bool bUseIncomingElement = true;

	/** Flat damage at ability level 0 (the base item level). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", EditCondition = "bEnableDamagePayload", EditConditionHides))
	float DamageValue = 0.f;

	/** Flat damage added per ability level (Value + Level * ValuePerLevel). */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", EditCondition = "bEnableDamagePayload", EditConditionHides))
	float DamageValuePerLevel = 0.f;

	/** Share of the triggering event's magnitude dealt: 0.1 = 10% of the damage
	 *  taken - the thorns / counter-attack case (Event.DamageTaken +
	 *  EventInstigator + this). 0 = pure flat damage. */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", EditCondition = "bEnableDamagePayload", EditConditionHides))
	float DamageRatio = 0.f;

	/** Extra share per ability level, on top of DamageRatio - the damage-family
	 *  convention (USnGA_DamageSkill::ScaleDamageByLevel):
	 *      Damage = EventMagnitude x (DamageRatio + Level x PerLevel) */
	UPROPERTY(EditDefaultsOnly, Category = "Damage", meta = (ClampMin = "0.0", EditCondition = "bEnableDamagePayload", EditConditionHides))
	float DamageRatioPerLevel = 0.f;

	// --- 3c. Payload: the child ability ---

	/** Tick to configure this payload - same switch pattern. Also gates the
	 *  pre-grant: a disabled child is never given a spec at all. */
	UPROPERTY(EditDefaultsOnly, Category = "TriggeredAbility", meta = (DisplayName = "启用子技能载荷"))
	bool bEnableAbilityPayload = false;

	/** Ability released on payout (e.g. a one-shot fireball). Pre-granted for
	 *  the whole active window and released with a plain TryActivateAbility, so
	 *  a refusal - its own cooldown still running, or an InstancedPerActor child
	 *  still active from the previous payout - is a SILENT SKIP, never a queue.
	 *  Make it a ServerOnly single-frame body (the USnGA_OneShotAttack family);
	 *  its own cooldown is what throttles a fast proc.
	 *
	 *  It runs on the listener's own ASC regardless of PayloadTargetMode - an
	 *  ability has no other way to activate. Spatial effects (hit the nearest
	 *  enemy, an area around a point) belong HERE, in the child's own shape. */
	UPROPERTY(EditDefaultsOnly, Category = "TriggeredAbility", meta = (EditCondition = "bEnableAbilityPayload", EditConditionHides))
	TSubclassOf<USnGameplayAbility> TriggeredAbilityClass;

	/** Description: "每 {N} 次{trigger}：{payloads}" composed from the configured
	 *  dimensions - the child's own description text (numbers live in the child
	 *  asset, same composition as the timed-trigger family) and / or the
	 *  catalog display name of the effect attribute with its level-scaled value,
	 *  and / or the damage payload's level-scaled value with its element. */
	virtual FText GetDescription_Implementation(int32 Level) override;

protected:
	/** Binds the trigger listener and pre-grants the child ability. */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** Unbinds the listener and revokes the child grant. */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** Counts one trigger; on reaching EventsPerTrigger, resets the counter and
	 *  pays out (latch-guarded, see the class doc). */
	void HandleTriggerEvent(FGameplayTag EventTag, const FGameplayEventData* Payload);

	/** Resolves PayloadTargetMode against the event's two actors (the listener
	 *  for Self, and when the payload's own actor is missing). */
	AActor* ResolvePayloadTargetActor(const FGameplayEventData* Payload) const;

	/** Payload A: applies the effect at GetAbilityLevel() to TargetActor, its
	 *  flat value plus EventMagnitude x EffectMagnitudeRatio. */
	void ApplyEffectPayload(float EventMagnitude, AActor* TargetActor);

	/** Payload B: applies the damage at GetAbilityLevel() to TargetActor, its
	 *  flat value plus EventMagnitude x DamageRatio. Marked as a riposte when it
	 *  answers Event.DamageTaken (see the class doc's termination note). */
	void ApplyDamagePayload(FGameplayTag EventTag, float EventMagnitude, const FSnGameplayEffectContext* EventContext, AActor* TargetActor);

	/** Payload C: releases the pre-granted child (silent skip on refusal). */
	void ReleaseTriggeredAbility();

private:
	/** Registered with EventFilter in ActivateAbility; removed (by the same
	 *  filter) in EndAbility. */
	FDelegateHandle EventDelegateHandle;

	/** The tag container the listener was registered with -
	 *  RemoveGameplayEventTagContainerDelegate matches by the same filter. */
	FGameplayTagContainer EventFilter;

	/** The child spec granted while this ability is active (paired with the
	 *  EndAbility removal). */
	FGameplayAbilitySpecHandle TriggeredAbilityHandle;

	/** Triggers seen since the last payout. Not replicated (see class doc). */
	int32 TriggerCount = 0;

	/** Re-entrancy latch: true while a payout is running, so a trigger this
	 *  payout itself raises cannot re-enter (see the class doc). */
	bool bExecutingPayload = false;
};
