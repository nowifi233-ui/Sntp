// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Characters/SntpCharacterBase.h"
#include "Interaction/CombatInterface.h"
#include "SntpPlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerCharacter : public ASntpCharacterBase, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	virtual void PossessedBy(AController* byController) override;
	virtual void OnRep_PlayerState() override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
protected:
	virtual void InitAbilityActorInfo() override;
	
};
