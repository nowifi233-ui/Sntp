// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "Interaction/CombatInterface.h"
#include "SntpCharacterBase.generated.h"

UCLASS()
class SNTP_API ASntpCharacterBase : public ACharacter, public ICombatInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASntpCharacterBase();
	
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual UAnimMontage* GetHitReacMontage_Implementation() override;
protected:
	virtual void BeginPlay() override;
	
	// GAS
	UPROPERTY(EditAnywhere, Category="GAS")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY(EditAnywhere, Category="GAS")
	TObjectPtr<UAttributeSet> AttributeSet;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	TObjectPtr<USkeletalMeshComponent> Weapon;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Combat")
	FName WeaponSocketName;
	
	virtual FVector GetCombatSocketLocation() override;
	
	virtual void InitAbilityActorInfo();
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS|DefaultAttribute")
	TSubclassOf<UGameplayEffect> DefaultPrimaryAttributes;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GAS|DefaultAttribute")
	TSubclassOf<UGameplayEffect> DefaultVitalAttributes;
	
	void InitializePrimaryAttributes();
	void InitializeVitalAttributes();
	virtual void InitializeDefaultAttributes() const {};
	
	void AddCharacterAbilities();
private:
	UPROPERTY(EditAnywhere, Category="GAS|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;
	
	UPROPERTY(EditAnywhere, Category="Combat")
	TObjectPtr<UAnimMontage> HitReactMontage;
};
