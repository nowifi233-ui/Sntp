// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

USnAttributeSet::USnAttributeSet()
{
	// Level starts at 1 (XP at 0): the level-up check consumes XP against this
	// baseline. Without this the panel would show 0 and the first level-up
	// would jump straight to 2.
	InitLevel(1.f);

	// Inventory starts at the default slot count; expansion GE modifiers raise it.
	InitCapacity(30.f);

	// Combat defaults (the rest start at 0; the default attributes GE can
	// override them). Crit/dodge baseline from the design (5% / 5%).
	InitCritChance(0.05f);
	InitCritDamage(0.5f);
	InitDodge(0.05f);

	// Movement speed defaults to the character's stock MaxWalkSpeed so nothing
	// changes until something modifies the attribute.
	InitMoveSpeed(600.f);

	// Shield deliberately starts - and stays - at 0 with no Init call: nothing
	// in the attribute set ever grants it. Skills and items add to it through
	// gameplay effects and it only comes down by being hit (see the Shield
	// member doc on the header).
	//
	// HealthRegen is the same shape: no Init, 0 until an effect grants it.
	// Regeneration is a stat a build carries, not a baseline assumed here.
}

void USnAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, MP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, MaxMP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, MPRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Shield, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, PhysicalDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, PhysicalDamagePercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, FireDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, IceDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, LightningDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, FireDamagePercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, IceDamagePercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, LightningDamagePercent, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, AttackSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Toughness, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, ArmorPenetration, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, CritChance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, CritDamage, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Dodge, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, FireResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, IceResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, LightningResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, XP, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, Capacity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(USnAttributeSet, CooldownReduction, COND_None, REPNOTIFY_Always);

	// Damage (the meta attribute) is deliberately NOT replicated: it is a
	// transient carrier resolved and reset within PostGameplayEffectExecute
	// on the server. Clients see only the resulting Health change.
}

void USnAttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Health, OldValue);
}

void USnAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, MaxHealth, OldValue);
}

void USnAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, HealthRegen, OldValue);
}

void USnAttributeSet::OnRep_MP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, MP, OldValue);
}

void USnAttributeSet::OnRep_MaxMP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, MaxMP, OldValue);
}

void USnAttributeSet::OnRep_MPRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, MPRegen, OldValue);
}

void USnAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Shield, OldValue);
}

void USnAttributeSet::OnRep_PhysicalDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, PhysicalDamage, OldValue);
}

void USnAttributeSet::OnRep_PhysicalDamagePercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, PhysicalDamagePercent, OldValue);
}

void USnAttributeSet::OnRep_FireDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, FireDamage, OldValue);
}

void USnAttributeSet::OnRep_IceDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, IceDamage, OldValue);
}

void USnAttributeSet::OnRep_LightningDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, LightningDamage, OldValue);
}

void USnAttributeSet::OnRep_FireDamagePercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, FireDamagePercent, OldValue);
}

void USnAttributeSet::OnRep_IceDamagePercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, IceDamagePercent, OldValue);
}

void USnAttributeSet::OnRep_LightningDamagePercent(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, LightningDamagePercent, OldValue);
}

void USnAttributeSet::OnRep_AttackSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, AttackSpeed, OldValue);
}

void USnAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, MoveSpeed, OldValue);
}

void USnAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Defense, OldValue);
}

void USnAttributeSet::OnRep_Toughness(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Toughness, OldValue);
}

void USnAttributeSet::OnRep_ArmorPenetration(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, ArmorPenetration, OldValue);
}

void USnAttributeSet::OnRep_CritChance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, CritChance, OldValue);
}

void USnAttributeSet::OnRep_CritDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, CritDamage, OldValue);
}

void USnAttributeSet::OnRep_Dodge(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Dodge, OldValue);
}

void USnAttributeSet::OnRep_FireResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, FireResistance, OldValue);
}

void USnAttributeSet::OnRep_IceResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, IceResistance, OldValue);
}

void USnAttributeSet::OnRep_LightningResistance(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, LightningResistance, OldValue);
}

void USnAttributeSet::OnRep_XP(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, XP, OldValue);
}

void USnAttributeSet::OnRep_Level(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Level, OldValue);
}

void USnAttributeSet::OnRep_Capacity(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, Capacity, OldValue);
}

void USnAttributeSet::OnRep_CooldownReduction(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(USnAttributeSet, CooldownReduction, OldValue);
}

void USnAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// Snapshot what the watched resource attributes hold right now, before this
	// write lands - see PreWriteValue on the header for why the value has to be
	// taken here and nowhere else. Positional: this runs for every write the
	// engine performs, and the effect-execution branches that read it (which
	// run immediately after) pick it up into a local first thing.
	if (Attribute == GetHealthAttribute() || Attribute == GetShieldAttribute() || Attribute == GetMPAttribute())
	{
		PreWriteValue = Attribute.GetNumericValue(this);
	}

	if (Attribute == GetHealthAttribute())
	{
		// The dead do not recover. Health regeneration is a periodic effect and
		// keeps ticking on a corpse: without this gate it would walk Health back
		// up from zero, and the next killing blow would find bIsDead already set
		// and broadcast nothing - the victim stops dying and every listener that
		// hangs off OnDeath (spawner stop, kill credit, loot) goes quiet with it.
		// The latch is the one thing that knows an actor is dead, so the gate
		// lives here rather than on the effect: heals are refused too, which is
		// the same statement. It lifts by itself - ResetDeath re-arms the latch
		// on respawn (ASnGameModeBase::ReviveDeadPlayers) and on pool reuse
		// (ASnEnemy::ConfigureForSpawn), and regeneration resumes with it.
		// FMath::Min, not a plain reject: Health must still be able to fall.
		if (bIsDead)
		{
			NewValue = FMath::Min(NewValue, GetHealth());
		}
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetMaxMPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetMPAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMP());
	}
	else if (Attribute == GetShieldAttribute())
	{
		// No cap to clamp against (the pool is uncapped by design), but never
		// below zero: the timed removal of a temporary shield subtracts its
		// full grant, and whatever was already spent has to die here - a
		// negative shield would otherwise absorb the next hit and heal.
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetHealthRegenAttribute())
	{
		// A rate, not a pool: negative regeneration would drain Health to zero
		// with no killer to credit, and the damage-over-time lane already exists
		// for anything that is supposed to hurt.
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetLevelAttribute())
	{
		NewValue = FMath::Max(NewValue, 1.f);
	}
	else if (Attribute == GetXPAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetCapacityAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetPhysicalDamageAttribute()
		|| Attribute == GetFireDamageAttribute()
		|| Attribute == GetIceDamageAttribute()
		|| Attribute == GetLightningDamageAttribute()
		|| Attribute == GetToughnessAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetPhysicalDamagePercentAttribute()
		|| Attribute == GetFireDamagePercentAttribute()
		|| Attribute == GetIceDamagePercentAttribute()
		|| Attribute == GetLightningDamagePercentAttribute()
		|| Attribute == GetAttackSpeedAttribute()
		|| Attribute == GetArmorPenetrationAttribute()
		|| Attribute == GetCritDamageAttribute()
		|| Attribute == GetFireResistanceAttribute()
		|| Attribute == GetIceResistanceAttribute()
		|| Attribute == GetLightningResistanceAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetDefenseAttribute())
	{
		// Armor points in the K/(K+D) curve - no upper bound, because mitigation
		// asymptotes to 100% on its own and stopping short of it is the point of
		// the curve. The floor is what the formula needs, not a balance cap: with
		// a negative defense the multiplier would climb past 1 and an attack
		// would heal its target.
		NewValue = FMath::Max(NewValue, 0.f);
	}
	else if (Attribute == GetDodgeAttribute() || Attribute == GetCritChanceAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 1.f);
	}
	else if (Attribute == GetCooldownReductionAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, 100.f);
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		NewValue = FMath::Max(NewValue, 0.f);
		SyncMoveSpeed(NewValue);
	}
}

void USnAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Copy the pre-write value BEFORE the clamp below: SetHealth re-enters
		// PreAttributeChange (which overwrites the member), and the gained event
		// at the end of this branch nests effect applications through proc
		// payloads. The local survives both.
		const float PreviousHealth = PreWriteValue;

		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));
		OnHealthChanged.Broadcast(GetHealth());

		// A heal that landed: the real amount, i.e. after the MaxHealth clamp
		// above and after the death latch had its say in PreAttributeChange
		// (a corpse gains nothing, so this fires nothing).
		SendAttributeGainedEvent(SnGameplayTags::Event_AttributeGained_Health, GetHealth() - PreviousHealth, Data.EffectSpec);

		// Death latch for direct-Health GEs only (heals, init): combat damage
		// resolves in the Damage branch above, which owns the kill credit for
		// the damage path. Both share bIsDead, so a hit can only latch once.
		if (GetHealth() <= 0.f && !bIsDead)
		{
			bIsDead = true;
			AActor* Killer = Data.EffectSpec.GetContext().GetInstigator();
			OnDeath.Broadcast(Killer);

			// The kill belongs to the KILLER's ASC - the one event that crosses
			// ability systems, so a "when I kill something" passive on their
			// equipment hears it wherever they are. Magnitude 0, see the tag doc.
			SendGameplayEvent(Killer, SnGameplayTags::Event_Kill, 0.f, Data.EffectSpec);
		}
	}
	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		// Shield went UP through a gameplay effect - the grant, never the spend.
		// Spending is invisible here by construction: absorbing a hit writes
		// Shield from the Damage branch below (an effect whose evaluated
		// attribute is Damage, not Shield), and the timed removal of a
		// temporary shield subtracts, which the sender filters out by sign.
		// No clamping either - PreAttributeChange already floored it at 0.
		const float PreviousShield = PreWriteValue;
		SendAttributeGainedEvent(SnGameplayTags::Event_AttributeGained_Shield, GetShield() - PreviousShield, Data.EffectSpec);
	}
	else if (Data.EvaluatedData.Attribute == GetMPAttribute())
	{
		// Same story as Shield: mana restored by an effect. The clamp to
		// [0, MaxMP] already happened in PreAttributeChange, so the delta is
		// the applied one.
		const float PreviousMP = PreWriteValue;
		SendAttributeGainedEvent(SnGameplayTags::Event_AttributeGained_MP, GetMP() - PreviousMP, Data.EffectSpec);
	}
	else if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// --- Incoming damage: apply the ExecCalc's result ---
		// The damage GE's execution (USnExecCalc_Damage) resolved everything -
		// dodge, elemental resistance, crit, toughness, defense - and wrote
		// the final amount on this meta attribute. Nothing left to resolve
		// here but the shield: zero the meta attribute, let the shield absorb
		// first, apply the overflow to Health, broadcast the result with the
		// resolution flags from the effect context. Being the last stop is what
		// keeps shield out of every mitigation above, and out of
		// USnCombatLibrary / USnExecCalc_Damage entirely.
		const FGameplayEffectContext* BaseContext = Data.EffectSpec.GetContext().Get();
		const FSnGameplayEffectContext* SnContext = static_cast<const FSnGameplayEffectContext*>(BaseContext);
		const bool bCritical = SnContext ? SnContext->IsCriticalHit() : false;
		const bool bMissed = SnContext ? SnContext->IsMissed() : false;
		const EDamageType DamageType = SnContext ? SnContext->GetDamageType() : EDamageType::Physical;

		const float IncomingDamage = GetDamage();
		SetDamage(0.f);
		if (IncomingDamage > 0.f)
		{
			// Shield eats first: it sits IN FRONT of Health, soaks up to
			// whatever it currently holds and only the overflow is felt.
			const float Absorbed = FMath::Min(GetShield(), IncomingDamage);
			if (Absorbed > 0.f)
			{
				SetShield(GetShield() - Absorbed);
			}
			const float RemainingDamage = IncomingDamage - Absorbed;

			if (RemainingDamage > 0.f)
			{
				const float NewHealth = FMath::Clamp(GetHealth() - RemainingDamage, 0.f, GetMaxHealth());
				SetHealth(NewHealth);

				// Kill credit + the one-shot death latch live HERE, not in the
				// Health branch: combat damage only ever rides the Damage meta
				// attribute (the GE has no Health modifier), so
				// PostGameplayEffectExecute(Health) never fires for it - without
				// this check enemies would reach 0 health and never die. The
				// Health branch still handles direct-Health GEs (heals, init),
				// and it cannot double-fire for the same hit (one GE never
				// carries both). Gated on the OVERFLOW rather than the raw hit:
				// a fully absorbed hit writes no Health and must not kill.
				if (NewHealth <= 0.f && !bIsDead)
				{
					bIsDead = true;
					AActor* Killer = Data.EffectSpec.GetContext().GetInstigator();
					OnDeath.Broadcast(Killer);

					// Cross-ASC hop, same as the Health branch's latch below.
					SendGameplayEvent(Killer, SnGameplayTags::Event_Kill, 0.f, Data.EffectSpec);
				}
			}
		}
		// Reports the full incoming amount, not the overflow: the damage number
		// is a readout of the hit, and the shield bar (fed by the Shield
		// attribute) is what shows the absorption.
		OnDamaged.Broadcast(IncomingDamage, bCritical, bMissed, DamageType);

		// Event.DamageTaken carries the very same number as the broadcast above -
		// deliberately, so a passive scaling off "the damage I took" agrees with
		// what the player saw. The victim-side counterpart of Event.AttackHit: the
		// resolved result exists nowhere else (the attacker only ever knows its
		// pre-mitigation value). Gated on landed damage: a fully avoided hit
		// (IncomingDamage == 0, dodge or full immunity) is not a hit taken.
		if (IncomingDamage > 0.f)
		{
			SendGameplayEvent(GetOwningActor(), SnGameplayTags::Event_DamageTaken, IncomingDamage, Data.EffectSpec);
		}
	}
}

float USnAttributeSet::GetXPRequiredForLevel(int32 CurrentLevel)
{
	// Level 1 needs 100 XP, level 2 needs 200, and so on.
	return FMath::Max(CurrentLevel, 1) * 100.f;
}

void USnAttributeSet::SendGameplayEvent(AActor* TargetActor, const FGameplayTag& EventTag, float Magnitude, const FGameplayEffectSpec& Spec) const
{
	if (!EventTag.IsValid() || !TargetActor)
	{
		return;
	}

	// The owner (the actor carrying this set) resolves to the component we are
	// already attached to without a lookup; anything else - the killer - goes
	// through the interface (ASnCharacter / ASnEnemy / ASnCompanion all
	// implement it, and the player's forwards to the PlayerState's ASC).
	const bool bToSelf = TargetActor == GetOwningActor();
	UAbilitySystemComponent* TargetASC = bToSelf
		? GetOwningAbilitySystemComponent()
		: UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(TargetActor);

	AActor* Owner = GetOwningActor();
	if (!TargetASC || !Owner || !Owner->HasAuthority())
	{
		return;
	}

	// Handed to the receiving ASC directly (HandleGameplayEvent, no multicast):
	// whoever reacts to this - the proc family - is an ability on that same ASC,
	// and what it does in response (granting an effect, triggering a child
	// ability) replicates on its own. The effect context rides along so a
	// listener can see the instigator / source object / element behind the event.
	FGameplayEventData Payload;
	Payload.EventTag = EventTag;
	Payload.Instigator = Spec.GetContext().GetInstigator();
	Payload.Target = TargetActor;
	Payload.EventMagnitude = Magnitude;
	Payload.OptionalObject = Spec.GetContext().GetSourceObject();
	Payload.ContextHandle = Spec.GetContext();
	TargetASC->HandleGameplayEvent(EventTag, &Payload);
}

void USnAttributeSet::SendAttributeGainedEvent(const FGameplayTag& EventTag, float Amount, const FGameplayEffectSpec& Spec)
{
	// Increases only. Spending shield, an expiring temporary shield and any
	// drain are subtractions and stay silent - a listener never has to filter
	// by sign to mean "when I gain X".
	if (Amount <= 0.f)
	{
		return;
	}

	SendGameplayEvent(GetOwningActor(), EventTag, Amount, Spec);
}

void USnAttributeSet::SyncMoveSpeed(float NewValue)
{
	AActor* Owner = GetOwningActor();
	if (!Owner)
	{
		return;
	}

	// Enemies own their ASC on the character; the player's ASC lives on the
	// PlayerState, so the pawn needs one extra hop.
	ACharacter* Character = Cast<ACharacter>(Owner);
	if (!Character)
	{
		if (const APlayerState* PS = Cast<APlayerState>(Owner))
		{
			Character = Cast<ACharacter>(PS->GetPawn());
		}
	}

	if (Character && Character->GetCharacterMovement())
	{
		Character->GetCharacterMovement()->MaxWalkSpeed = NewValue;
	}
}
