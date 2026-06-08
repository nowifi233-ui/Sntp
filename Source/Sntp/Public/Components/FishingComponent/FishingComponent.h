// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FishingComponent.generated.h"

class UItemDefinition;

UENUM(BlueprintType)
enum class EFishingState : uint8
{
	Idle,
	Fighting,
	Success,
	Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnFishingUIUpdate,
	float, FishPosition,
	float, BarPosition,
	float, CatchProgress
);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FOnFishDebug,
	float, FishTarget,
	float, FishSpeed,
	float, FishMoveTimer);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFishSuccess);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UFishingComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UFishingComponent();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
public:
	UPROPERTY(BlueprintReadWrite)
	EFishingState FishingState = EFishingState::Idle;
	
	// UI
	UPROPERTY(BlueprintAssignable)
	FOnFishingUIUpdate OnFishingUIUpdate;
	
	UPROPERTY(BlueprintAssignable)
	FOnFishDebug OnFishDebug;
	
	UPROPERTY(BlueprintAssignable)
	FOnFishSuccess OnFishSuccess;
	
	UPROPERTY(BlueprintAssignable)
	FOnFishSuccess OnFishFailed;
	
	UPROPERTY(BlueprintAssignable)
	FOnFishSuccess OnFishClosed;
	
	// player bar
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Bar")
	float BarPosition = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Bar")
	float BarVelocity = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Bar")
	float Gravity = -10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Bar")
	float InputForce = 4.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Bar")
	float Damping = 0.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Bar")
	float BarHeight = 0.2f;
	
	// Fish
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Fish")
	float FishPosition = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Fish")
	float FishSpeed = 0.3f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Fish")
	float FishMoveInterval = 1.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Fish")
	UItemDefinition* FishingItemDefinition;
	
	// Catch
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Catch")
	float CatchProgress = 0.5f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Catch")
	float CatchIncreaseSpeed = 0.35f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fishing|Catch")
	float CatchDecreaseSpeed = 0.25f;
	
public:
	
	UFUNCTION(BlueprintCallable, Category="Fishing|Fish")
	void StartFishing();
	
	UFUNCTION(BlueprintCallable, Category="Fishing|Fish")
	void StopFishing();
	
	UFUNCTION(BlueprintCallable, Category="Fishing|Fish")
	void SetInputPressed(bool bPressed);
	
	UFUNCTION()
	void FishingSuccess();
	
	UFUNCTION(BlueprintCallable, Category="Fishing|Fish")
	void CloseFishing();

protected:
	void UpdateBar(float DeltaTime);
	
	void UpdateFish(float DeltaTime);
	
	void UpdateCatchProgress(float DeltaTime);
	
	bool IsFishInsideBar() const;
	
private:
	bool bHoldingInput = false;
	float FishTargetPosition = 0.5f;
	float FishMoveTimer = 0.f;
};
