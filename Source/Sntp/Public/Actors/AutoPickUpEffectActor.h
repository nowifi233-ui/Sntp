// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/EffectActor.h"
#include "AutoPickUpEffectActor.generated.h"

class UProjectileMovementComponent;
class UFloatingPawnMovement;
/**
 * 
 */
UCLASS()
class SNTP_API AAutoPickUpEffectActor : public AEffectActor
{
	GENERATED_BODY()
public:
	AAutoPickUpEffectActor();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void Tick(float DeltaSeconds) override;
	
	// Float 
	UPROPERTY(EditAnywhere, Category="Pickup")
	float FloatSpeed = 500.f;
	
	UPROPERTY(EditAnywhere, Category="Pickup")
	float FloatAmplitude = 1.5f;
	
	UFUNCTION(BlueprintCallable, Category="Pickup")
	void StartAttract(AActor* InTargetActor);
	
	UFUNCTION()
	void UpdateItem();
	
	UPROPERTY(EditAnywhere, Category="Attract")
	float AttractSpeed;
	
	//Timer
	FTimerHandle UpdateTimer;

private:
	bool bStartAttract = false;
	FVector StartLocation;
	UPROPERTY()
	AActor* TargetPlayer;
	
	UPROPERTY()
	TObjectPtr<AActor> CurrentTarget = nullptr;
	
	/*UPROPERTY(VisibleAnywhere)
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;*/
};
