// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/HealthTower.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "SntpGameplayTags.h"

class UAbilitySystemComponent;
class IAbilitySystemInterface;

TArray<FInteractionOption> AHealthTower::GetInteractionOptions()
{
	return Options;
}

void AHealthTower::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == "Heal")
	{
		GEngine->AddOnScreenDebugMessage(0, 5, FColor::Red, FString::Printf(TEXT("Ride Interact")), false);
		if (IAbilitySystemInterface* ASCI = Cast<IAbilitySystemInterface>(Interactor))
		{
			if (DamageEffectClass == nullptr)
			{
				return;
			}
			FSntpGameplayTags GameplayTags = FSntpGameplayTags::Get();
			UAbilitySystemComponent* InteractorASC = ASCI->GetAbilitySystemComponent();
			FGameplayEffectContextHandle ContextHandle = InteractorASC->MakeEffectContext();
			FGameplayEffectSpecHandle EffectSpecHandle = InteractorASC->MakeOutgoingSpec(DamageEffectClass, 1.0f, ContextHandle);
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, GameplayTags.Damage, -114514.f);
			InteractorASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}
}
