// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Data/CharacterClassInfo.h"
#include "Characters/SntpCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "SntpEnemyCharacter.generated.h"

enum class ECharacterClass : uint8;
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
	

	UPROPERTY(BlueprintAssignable)
	FOnAttributeSignature OnMaxHealthChange;
	
	UPROPERTY(BlueprintAssignable)
	FOnAttributeSignature OnHealthChange;
	
	/*
	 * Hit react
	 */
	void HitReactTagChanged(const FGameplayTag CallbackTag, int32 NewCount);
	
	UPROPERTY(BlueprintReadWrite, Category="Combat")
	bool bHitReacting = false;
	
	UPROPERTY(BlueprintReadWrite, Category="Combat")
	float BaseWalkSpeed = 250.f;
	
protected:
	virtual void InitAbilityActorInfo() override;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Class Default")
	int32 Level = 1.0f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Character Class Default")
	ECharacterClass CharacterClass = ECharacterClass::Warrior;
	
	virtual void InitializeDefaultAttributes() const override;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UWidgetComponent> HealthBarComponent;
};
