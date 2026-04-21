// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/CharacterClassInfo.h"
#include "Characters/SntpCharacterBase.h"
#include "Interaction/EnemyInterface.h"
#include "UI/WidgetController/OverlayWidgetController.h"
#include "SntpEnemyCharacter.generated.h"

class APickupActor;
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
	virtual void Die();

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
	
	/* Drop */
public:
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TObjectPtr<UItemDefinition> DropItemDef;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<APickupActor> PickupActorClass;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 DropAmount = 1;
	
	UFUNCTION(BlueprintCallable)
	virtual void Drop() override;
};
