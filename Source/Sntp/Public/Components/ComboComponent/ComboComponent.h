// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ComboDataAsset.h"
#include "Components/ActorComponent.h"
#include "ComboComponent.generated.h"


class UAbilitySystemComponent;
class UComboDataAsset;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UComboComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UComboComponent();
	
	UFUNCTION(BlueprintCallable)
	void Init(UAbilitySystemComponent* InASC, UComboDataAsset* InData);
	
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
};
