// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/USnCombatLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Combat/ASnDestructible.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayEffectContext.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameplayEffect.h"

bool USnCombatLibrary::IsEnemy(AActor* InActor)
{
	if (!InActor)
	{
		return false;
	}

	// Tag lives on the ASC (GAS convention: UI and effects can read it too).
	// HasMatchingGameplayTag matches child tags as well, so a future
	// Combat.Team.Enemy.Melee variant is recognized without changes.
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InActor))
	{
		return ASC->HasMatchingGameplayTag(SnGameplayTags::Combat_Team_Enemy);
	}
	return false;
}

ASnDestructible* USnCombatLibrary::FindDestructible(AActor* InActor)
{
	if (!InActor)
	{
		return nullptr;
	}

	// Plain cast: destructibles are ordinary actors with Pawn-typed collision
	// (no tag, no ASC) - the class IS the marker.
	return Cast<ASnDestructible>(InActor);
}

AActor* USnCombatLibrary::FindNearestEnemy(UObject* WorldContextObject, FVector Center, float Radius)
{
	AActor* Best = nullptr;
	float BestDistanceSq = MAX_FLT;
	for (AActor* Enemy : GetEnemiesInRadius(WorldContextObject, Center, Radius))
	{
		const float DistanceSq = FVector::DistSquared(Center, Enemy->GetActorLocation());
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			Best = Enemy;
		}
	}
	return Best;
}

TArray<AActor*> USnCombatLibrary::GetEnemiesInRadius(UObject* WorldContextObject, FVector Center, float Radius)
{
	TArray<AActor*> Enemies;

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World || Radius <= 0.f)
	{
		return Enemies;
	}

	// Overlap all pawns (companion/player included - the IsEnemy tag filter
	// drops anything not marked hostile), then keep the tagged ones.
	FCollisionQueryParams QueryParams(FName(TEXT("SnCombatLibraryEnemies")), /*bTraceComplex=*/false);
	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByChannel(Overlaps, Center, FQuat::Identity, ECC_Pawn,
		FCollisionShape::MakeSphere(Radius), QueryParams);

	for (const FOverlapResult& Result : Overlaps)
	{
		AActor* HitActor = Result.GetActor();
		if (HitActor && IsEnemy(HitActor))
		{
			Enemies.Add(HitActor);
		}
	}
	return Enemies;
}

FSnDamageResult USnCombatLibrary::CalculateDamage(UAbilitySystemComponent* SourceASC, EDamageType DamageType)
{
	FSnDamageResult Result;
	Result.DamageType = DamageType;
	if (!SourceASC)
	{
		return Result;
	}

	// base[type] x (1 + percent[type]). No crit here: the attacker's roll
	// moved into USnExecCalc_Damage (it captures CritChance/CritDamage on the
	// source), so damage application and damage resolution are decoupled - a
	// skill can combine numbers freely and still route through one mitigation
	// pipeline.
	switch (DamageType)
	{
	case EDamageType::Physical:
		Result.Damage = SourceASC->GetNumericAttribute(USnAttributeSet::GetPhysicalDamageAttribute())
			* (1.f + SourceASC->GetNumericAttribute(USnAttributeSet::GetPhysicalDamagePercentAttribute()));
		break;
	case EDamageType::Fire:
		Result.Damage = SourceASC->GetNumericAttribute(USnAttributeSet::GetFireDamageAttribute())
			* (1.f + SourceASC->GetNumericAttribute(USnAttributeSet::GetFireDamagePercentAttribute()));
		break;
	case EDamageType::Ice:
		Result.Damage = SourceASC->GetNumericAttribute(USnAttributeSet::GetIceDamageAttribute())
			* (1.f + SourceASC->GetNumericAttribute(USnAttributeSet::GetIceDamagePercentAttribute()));
		break;
	case EDamageType::Lightning:
		Result.Damage = SourceASC->GetNumericAttribute(USnAttributeSet::GetLightningDamageAttribute())
			* (1.f + SourceASC->GetNumericAttribute(USnAttributeSet::GetLightningDamagePercentAttribute()));
		break;
	}

	return Result;
}

FGameplayEffectSpecHandle USnCombatLibrary::MakeDamageSpec(UAbilitySystemComponent* SourceASC, TSubclassOf<UGameplayEffect> DamageEffectClass, AActor* Instigator, float Damage, EDamageType DamageType, float AbilityLevel, bool bReflected)
{
	if (!SourceASC || !DamageEffectClass || !Instigator)
	{
		return FGameplayEffectSpecHandle();
	}

	// Instigator = the attacking actor: the victim's kill credit and effect
	// context read it when the damage GE executes. The element flag rides on
	// the context too (every context is an FSnGameplayEffectContext via
	// USnAbilitySystemGlobals); the ExecCalc reads it to pick the resistance
	// and the SetByCaller key. Both MUST be set here in C++ - a blueprint
	// cannot reach SetDamageType (native-only), and a missing instigator
	// silently kills projectile delivery (the hit-time lookup).
	FGameplayEffectContextHandle Context = SourceASC->MakeEffectContext();
	Context.AddInstigator(Instigator, Instigator);
	Context.AddSourceObject(Instigator);
	if (FSnGameplayEffectContext* SnContext = static_cast<FSnGameplayEffectContext*>(Context.Get()))
	{
		SnContext->SetDamageType(DamageType);
		SnContext->SetIsReflected(bReflected);
	}

	const FGameplayEffectSpecHandle Spec = SourceASC->MakeOutgoingSpec(DamageEffectClass, AbilityLevel, Context);
	if (!Spec.IsValid())
	{
		return FGameplayEffectSpecHandle();
	}

	// Negative magnitude: the damage GE adds it to the Damage meta attribute
	// (GAS has no subtract op). One key per element - the ExecCalc resolves
	// it. Penetration and the crit flag no longer travel here: penetration is
	// captured from the source by the ExecCalc, and the crit roll lives there
	// too.
	Spec.Data->SetSetByCallerMagnitude(GetDamageSetByCallerKey(DamageType), -Damage);
	return Spec;
}

bool USnCombatLibrary::ApplyDamageToTarget(UAbilitySystemComponent* SourceASC, TSubclassOf<UGameplayEffect> DamageEffectClass, AActor* Instigator, UAbilitySystemComponent* TargetASC, const FSnDamageResult& Damage, float AbilityLevel)
{
	if (!SourceASC || !DamageEffectClass || !TargetASC)
	{
		return false;
	}

	// Immediate application of the very spec MakeDamageSpec builds for delayed
	// delivery (projectiles): one shared spec construction, one pipeline.
	const FGameplayEffectSpecHandle Spec = MakeDamageSpec(SourceASC, DamageEffectClass, Instigator, Damage.Damage, Damage.DamageType, AbilityLevel, Damage.bReflected);
	if (!Spec.IsValid())
	{
		return false;
	}

	SourceASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
	return true;
}

FGameplayTag USnCombatLibrary::GetDamageSetByCallerKey(EDamageType DamageType)
{
	switch (DamageType)
	{
	case EDamageType::Fire:
		return SnGameplayTags::SetByCaller_Damage_Fire;
	case EDamageType::Ice:
		return SnGameplayTags::SetByCaller_Damage_Ice;
	case EDamageType::Lightning:
		return SnGameplayTags::SetByCaller_Damage_Lightning;
	default:
		return SnGameplayTags::SetByCaller_Damage_Physical;
	}
}

FName USnCombatLibrary::GetDamageNumberTag(EDamageType DamageType)
{
	// Row names in the shared damage-number style table (WBP_DamageNumber's
	// TextStyleSet). A row missing from the table falls back to its
	// DefaultTextStyle in the widget, so an unconfigured element still renders.
	switch (DamageType)
	{
	case EDamageType::Fire:
		return TEXT("Damage.Fire");
	case EDamageType::Ice:
		return TEXT("Damage.Ice");
	case EDamageType::Lightning:
		return TEXT("Damage.Lightning");
	default:
		return TEXT("Damage.Physical");
	}
}

FText USnCombatLibrary::GetDamageTypeDisplayName(EDamageType DamageType)
{
	switch (DamageType)
	{
	case EDamageType::Fire:
		return FText::FromString(TEXT("火焰"));
	case EDamageType::Ice:
		return FText::FromString(TEXT("冰霜"));
	case EDamageType::Lightning:
		return FText::FromString(TEXT("闪电"));
	default:
		return FText::FromString(TEXT("物理"));
	}
}

FName USnCombatLibrary::GetDamageTypeStyleTag(EDamageType DamageType)
{
	// Row names in the shared DT_TextStyles table (the ability-description
	// counterpart of the damage number's Damage.* tags). A row missing from
	// the table falls back to the default style, so an unconfigured element
	// still renders.
	switch (DamageType)
	{
	case EDamageType::Fire:
		return TEXT("Element.Fire");
	case EDamageType::Ice:
		return TEXT("Element.Ice");
	case EDamageType::Lightning:
		return TEXT("Element.Lightning");
	default:
		return TEXT("Element.Physical");
	}
}
