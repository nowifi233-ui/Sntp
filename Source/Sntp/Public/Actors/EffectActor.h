// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "GameplayEffectTypes.h"
#include "EffectActor.generated.h"

class UAbilitySystemComponent;
class UGameplayEffect;

UENUM(BlueprintType)
enum class EEffectApplyPolicy: uint8
{
	DoNotApply,
	ApplyOnOverlap,
	ApplyOnEndOverlap
};

UENUM(BlueprintType)
enum class EEffectRemovalPolicy: uint8
{
	Remove,
	DoNotRemove
};

UCLASS()
class SNTP_API AEffectActor : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AEffectActor();
	
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	bool bDestroyOnApply = false;
	/**
	 * EffectClass, set in blueprints.
	 */
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> InstantGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> DurationGameplayEffectClass;
	
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	TSubclassOf<UGameplayEffect> InfiniteGameplayEffectClass;

	/**
	 * Gameplay Effect Policies, set it in blueprints
	 */
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	EEffectApplyPolicy InstantEffectApplyPolicy;
	
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	EEffectApplyPolicy DurationEffectApplyPolicy;
	
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	EEffectApplyPolicy InfiniteEffectApplyPolicy;
	
	UPROPERTY(EditAnywhere, Category="GE", BlueprintReadOnly)
	EEffectRemovalPolicy InfiniteEffectRemovalPolicy;

	/**
	 *  Call those functions in blueprints.
	 */
	UFUNCTION(BlueprintCallable)
	void OnOverlap(AActor* Target);
	
	UFUNCTION(BlueprintCallable)
	void OnEndOverlap(AActor* Target);
	
	TMap<FActiveGameplayEffectHandle, UAbilitySystemComponent*> ActiveEffectHandles;
protected:
	
	void ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass);
	
	
};
