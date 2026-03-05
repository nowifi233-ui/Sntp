// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/SntpInputConfig.h"

const UInputAction* USntpInputConfig::FindInputActionByTag(const FGameplayTag& InputTag, bool bLogNotFound) const
{
	for (const FSntpInputAction& SntpInputAction : SntpInputActions)
	{
		if (InputTag.MatchesTagExact(SntpInputAction.InputTag))
		{
			return SntpInputAction.InputAction;
		}
	}
	if (bLogNotFound)
	{
		UE_LOG(LogTemp, Error, TEXT("Can't find AbilityInputAction for InputTag [%s], on InputConfig [%s]"), *InputTag.ToString(), *GetNameSafe(this));
	}
	return nullptr;
}
