// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "SntpPlayerState.generated.h"

class UAttributeSet;

/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	ASntpPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	UAttributeSet* GetAttributeSet() const;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
	
public:

	int64 GetPlayerUid() const;

	const FString& GetPlayerDisplayName() const;

private:

	UPROPERTY(Replicated)
	int64 PlayerUid = INDEX_NONE;

	UPROPERTY(Replicated)
	FString PlayerDisplayName;
	
};
