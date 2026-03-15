// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/EffectActor.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystem/SntpAttributeSet.h"

// Sets default values
AEffectActor::AEffectActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	SetRootComponent(CreateDefaultSubobject<USceneComponent>("SceneRoot"));
}

void AEffectActor::ApplyEffectToTarget(AActor* Target, TSubclassOf<UGameplayEffect> GameplayEffectClass)
{	
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (TargetASC == nullptr)
	{
		return;
	}
	check(GameplayEffectClass);
	FGameplayEffectContextHandle EffectContextHandle = TargetASC->MakeEffectContext();
	EffectContextHandle.AddSourceObject(this);
	const FGameplayEffectSpecHandle EffectSpecHandle = TargetASC->MakeOutgoingSpec(GameplayEffectClass, ActorLevel, EffectContextHandle);
	const FActiveGameplayEffectHandle ActiveEffectHandle = TargetASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
	
	const bool bIsInfinite = EffectSpecHandle.Data.Get()->Def.Get()->DurationPolicy == EGameplayEffectDurationType::Infinite;
	if (bIsInfinite)
	{
		ActiveEffectHandles.Add(ActiveEffectHandle, TargetASC);
	}
	
	if (bDestroyOnApply)
	{
		Destroy();
	}
}


void AEffectActor::OnOverlap(AActor* Target)
{
	if (InstantEffectApplyPolicy == EEffectApplyPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(Target, InstantGameplayEffectClass);
	}
	if (DurationEffectApplyPolicy == EEffectApplyPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(Target, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplyPolicy == EEffectApplyPolicy::ApplyOnOverlap)
	{
		ApplyEffectToTarget(Target, InfiniteGameplayEffectClass);
	}
}

void AEffectActor::OnEndOverlap(AActor* Target)
{
	if (InstantEffectApplyPolicy == EEffectApplyPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(Target, InstantGameplayEffectClass);
	}
	if (DurationEffectApplyPolicy == EEffectApplyPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(Target, DurationGameplayEffectClass);
	}
	if (InfiniteEffectApplyPolicy == EEffectApplyPolicy::ApplyOnEndOverlap)
	{
		ApplyEffectToTarget(Target, InfiniteGameplayEffectClass);
	}
	if (InfiniteEffectRemovalPolicy == EEffectRemovalPolicy::Remove)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
		if (TargetASC == nullptr)
		{
			return;
		}
		TArray<FActiveGameplayEffectHandle> HandlesToRemove;
		for (TTuple<FActiveGameplayEffectHandle, UAbilitySystemComponent*> tuple : ActiveEffectHandles)
		{
			if (TargetASC == tuple.Value)
			{
				TargetASC->RemoveActiveGameplayEffect(tuple.Key);
				HandlesToRemove.Add(tuple.Key);
			}
		}
		for (auto& Handle : HandlesToRemove)
		{
			ActiveEffectHandles.Remove(Handle);
		}
	}
}



