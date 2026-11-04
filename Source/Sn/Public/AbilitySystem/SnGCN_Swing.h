// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayCueNotify_Burst.h"
#include "SnGCN_Swing.generated.h"

class UCameraShakeBase;

/**
 * Swing-cue notify base (GameplayCue.Swing.<element>): a data-driven engine
 * UGameplayCueNotify_Burst (arc + whoosh configured on the asset's
 * BurstEffects - pure Details panel, no graph) extended with the
 * once-per-swing hit camera shake.
 *
 * The engine's BurstCameraShake slot always plays on execution - it has no
 * "something was hit" condition - so it stays empty on the assets and the
 * shake runs here instead, gated on the cue parameters the attack core
 * (USnGA_SphereAttack::ExecuteSphereAttack) sends: RawMagnitude > 0 (the
 * swing hit something - a miss still plays the arc and whoosh but never
 * shakes) and the cue's Instigator being a locally controlled pawn (the
 * notify runs on every side that sees the cue; on observer machines the
 * instigator is not local, so only the attacker's own machine shakes - the
 * same rule the engine's burst camera shake applies via its
 * LocallyControlledPolicy). HitShakeClass / HitShakeScale are configured per
 * asset.
 *
 * Assets: GC_Swing_Melee / GC_Swing_Fire derive from this class. The hit-cue
 * assets (GC_Hit_*) carry no logic at all - they derive straight from the
 * engine's UGameplayCueNotify_Burst.
 */
UCLASS(Blueprintable, Category = "GameplayCueNotify")
class SN_API USnGCN_Swing : public UGameplayCueNotify_Burst
{
	GENERATED_BODY()

protected:
	virtual bool OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const override;

	/** Camera shake played once per swing that hit something (see the class
	 *  doc for the gates). Empty = never shake. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCueNotify")
	TSubclassOf<UCameraShakeBase> HitShakeClass;

	/** Intensity of the hit camera shake. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "GameplayCueNotify")
	float HitShakeScale = 1.f;
};
