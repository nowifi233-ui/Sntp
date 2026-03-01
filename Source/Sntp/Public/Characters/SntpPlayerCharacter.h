// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Characters/SntpCharacterBase.h"
#include "SntpPlayerCharacter.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerCharacter : public ASntpCharacterBase
{
	GENERATED_BODY()
public:
	virtual void PossessedBy(AController* byController) override;
	virtual void OnRep_PlayerState() override;
	
private:
	void InitAbilityActorInfo();
};
