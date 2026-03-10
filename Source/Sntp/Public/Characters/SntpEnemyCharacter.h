// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/SntpCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "SntpEnemyCharacter.generated.h"

class UWidgetComponent;
/**
 * 
 */
UCLASS()
class SNTP_API ASntpEnemyCharacter : public ASntpCharacterBase, public IEnemyInterface
{
	GENERATED_BODY()
	
public:
	ASntpEnemyCharacter();
	virtual void HighlightActor() override;
	virtual void UnHighlightActor() override;
	
	virtual void BeginPlay() override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UWidgetComponent> HealthBarComponent;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeSignature OnMaxHealthChange;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeSignature OnHealthChange;
	
protected:
	virtual void InitAbilityActorInfo() override;
};
