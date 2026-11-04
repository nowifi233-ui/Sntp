// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/SnGA_Summon.h"
#include "SnGA_SummonCompanion.generated.h"

/**
 * Summon companion (shell): the summon-family core (USnGA_Summon) under the
 * ASnCompanion contract of the original class - the class name is preserved
 * so the existing companion blueprints need no reparent, only the property
 * move: point SummonClass (was CompanionClass) at the companion BP and set
 * SpawnRadius on the class defaults.
 *
 * The companion's combat strength is its own (the companion blueprint), so
 * nothing here scales with level. The death-ends-the-ability window of the
 * core closes the summon when the companion dies in combat - re-summoning
 * then runs through the same entry as before (avatar set: re-equip / level
 * travel, or a sustain trigger's next release).
 */
UCLASS()
class SN_API USnGA_SummonCompanion : public USnGA_Summon
{
	GENERATED_BODY()

public:
	/** Description (static: the companion's combat strength is its own - the
	 *  companion actor - so nothing here scales with level). */
	virtual FText GetDescription_Implementation(int32 Level) override;
};
