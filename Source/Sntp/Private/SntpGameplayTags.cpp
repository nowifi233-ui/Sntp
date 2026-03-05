// Fill out your copyright notice in the Description page of Project Settings.


#include "SntpGameplayTags.h"
#include "GameplayTagsManager.h"

FSntpGameplayTags FSntpGameplayTags::GameplayTags;

void FSntpGameplayTags::InitializeNativeGameplayTags()
{
	 GameplayTags.Attributes_Vital_MaxHealth = UGameplayTagsManager::Get().AddNativeGameplayTag(
		FName("Attributes.Vital.MaxHealth"),
		FString("MaxHealth")
	);
	
	
}



