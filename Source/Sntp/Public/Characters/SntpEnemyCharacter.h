// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/SntpCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "SntpEnemyCharacter.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API ASntpEnemyCharacter : public ASntpCharacterBase, public IEnemyInterface
{
	GENERATED_BODY()
	
public:
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
	
};
