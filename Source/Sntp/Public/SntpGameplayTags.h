// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * Singleton containing native gameplay tags.
 */

struct FSntpGameplayTags
{
public:
	static const FSntpGameplayTags& Get() {return GameplayTags;}
	static void InitializeNativeGameplayTags();
	
	FGameplayTag Attributes_Vital_MaxHealth;
private:
	
protected:
	static FSntpGameplayTags GameplayTags;
};