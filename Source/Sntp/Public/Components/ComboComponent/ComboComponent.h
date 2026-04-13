// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComboDataAsset.h"
#include "GameplayEffectTypes.h"
#include "Components/ActorComponent.h"
#include "ComboComponent.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;
class UComboDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UComboComponent();
	
	UFUNCTION(BlueprintCallable)
	void Init(UAbilitySystemComponent* InASC);
	
	UFUNCTION(BlueprintCallable)
	void OnInput(EInputType Input);
	
	UFUNCTION(BlueprintCallable)
	void OpenComboWindow();
	
	UFUNCTION(BlueprintCallable)
	void CloseComboWindow();
	
	UFUNCTION(BlueprintCallable)
	void OpenBufferWindow() {bBufferWindowOpen = true;}
	
	UFUNCTION(BlueprintCallable)
	void CloseBufferWindow() {bBufferWindowOpen = false;}
	
	UFUNCTION(BlueprintCallable)
	void ResetCombo();
	
	const FComboNode* GetNode(FName NodeID) const;
	float GetCurrentInputWindow() const;
	void ExecuteInput(EInputType Input);
	void TryConsumeInput();
	int32 GetPriority(EInputType Input) const;
	void StartCombo(EInputType Input);
	
	UFUNCTION(BlueprintCallable)
	bool GetComboWindowOpen() {return bComboWindowOpen; }
	
	UFUNCTION(BlueprintCallable)
	void Interrupt();
private:
	UPROPERTY()
	UAbilitySystemComponent* ASC;
	
	UPROPERTY(EditAnywhere)
	UComboDataAsset* ComboData;
	
	FName CurrentNode;
	FPendingInput PendingInput;
	bool bComboWindowOpen = false; 
	bool bBufferWindowOpen = false;
	bool bIsInCombo = false;
	
	/**
	 * Weapon Trace
	 */
public:
	void HitScan();
	
	UFUNCTION(BlueprintCallable)
	void HitScanStart(FGameplayEffectSpecHandle DamageEffectSpecHandle);
	
	UFUNCTION(BlueprintCallable)
	void HitScanEnd();
	
	UPROPERTY()
	FTimerHandle WeaponHitTimer;
	
	UPROPERTY()
	TArray<AActor*> HitActors;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TSubclassOf<UGameplayEffect> DamageEffectClass;
private:
	FGameplayEffectSpecHandle WeaponTraceEffectHandle;
	FVector LastWeaponStart;
	FVector LastWeaponMid;
	FVector LastWeaponEnd;
	bool bHasLastFrame = false; 
};
