// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_DamageSkill.h"

#include "Combat/USnCombatLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

FRotator USnGA_DamageSkill::GetAvatarAimRotation(const AActor* Avatar)
{
	if (!Avatar)
	{
		return FRotator::ZeroRotator;
	}

	// Only a player's pawn has a view to aim by - see the header. The controller
	// (not the pawn) owns the aim: on the authority side a remote client's
	// control rotation arrives through its movement RPCs.
	const APawn* Pawn = Cast<APawn>(Avatar);
	const APlayerController* PC = Pawn ? Cast<APlayerController>(Pawn->GetController()) : nullptr;

	// No player controller (AI enemies, companions): the actor's own facing is
	// the aim - the yaw its AI already turned it to.
	const FRotator Source = PC ? PC->GetControlRotation() : Avatar->GetActorRotation();
	return FRotator(0.f, Source.Yaw, 0.f);
}

float USnGA_DamageSkill::ScaleDamageByLevel(float Damage, float Multiplier, float PerLevelGrowth, int32 Level)
{
	return Damage * (Multiplier + FMath::Max(0, Level) * PerLevelGrowth);
}

FText USnGA_DamageSkill::BuildDamageClause(int32 Level, float Multiplier, float PerLevelGrowth, EDamageType DamageType)
{
	// The same growth expression ScaleDamageByLevel uses, turned into the
	// percent the UI shows - level clamped the same way, so the panel number
	// always matches the gameplay number. <...> names the element's row in
	// the shared DT_TextStyles table (missing row falls back to default).
	const float EffectiveMultiplier = Multiplier + FMath::Max(0, Level) * PerLevelGrowth;
	const int32 Percent = FMath::RoundToInt(EffectiveMultiplier * 100.f);
	return FText::Format(
		FText::FromString(TEXT("{0}% 的<{1}>{2}</>伤害")),
		Percent,
		FText::FromName(USnCombatLibrary::GetDamageTypeStyleTag(DamageType)),
		USnCombatLibrary::GetDamageTypeDisplayName(DamageType));
}
