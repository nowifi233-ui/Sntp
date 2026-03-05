// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/GameplayAbilities/SntpGameplayAbility.h"
#include "SntpProjectileSpell.generated.h"

class ASntpProjectile;
/**
 * 
 */
UCLASS()
class SNTP_API USntpProjectileSpell : public USntpGameplayAbility
{
	GENERATED_BODY()
protected:
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION(BlueprintCallable)
	void SpawnProjectile();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TSubclassOf<ASntpProjectile> ProjectileClass;

};
