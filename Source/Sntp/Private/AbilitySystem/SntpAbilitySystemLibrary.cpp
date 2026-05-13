// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/SntpAbilitySystemLibrary.h"

#include "AbilitySystemComponent.h"
#include "SntpAbilityTypes.h"
#include "SntpGameplayTags.h"
#include "Data/CharacterClassInfo.h"
#include "Game/SntpGameModeBase.h"
#include "Interaction/Interactable.h"
#include "Kismet/GameplayStatics.h"

void USntpAbilitySystemLibrary::InitializeDefaultAttributes(const UObject* WorldContextObject,
                                                            ECharacterClass CharacterClass, float Level, UAbilitySystemComponent* ASC)
{
	ASntpGameModeBase* SntpGameMode = Cast<ASntpGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (SntpGameMode == nullptr)
	{
		return;
	}
	UCharacterClassInfo* ClassInfo = SntpGameMode->CharacterClassInfo;
	const FCharacterClassDefaultInfo CharacterClassInfo = ClassInfo->GetClassDefaultInfo(CharacterClass);
	
	const FGameplayEffectSpecHandle PrimaryEffectSpecHandle = ASC->MakeOutgoingSpec(CharacterClassInfo.PrimaryAttribute, Level, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*PrimaryEffectSpecHandle.Data.Get());
	
	const FGameplayEffectSpecHandle VitalEffectSpecHandle = ASC->MakeOutgoingSpec(ClassInfo->VitalAttribute , Level, ASC->MakeEffectContext());
	ASC->ApplyGameplayEffectSpecToSelf(*VitalEffectSpecHandle.Data.Get());
}

void USntpAbilitySystemLibrary::GiveStartupAbilites(const UObject* WorldContextObject, UAbilitySystemComponent* ASC)
{
	ASntpGameModeBase* SntpGameMode = Cast<ASntpGameModeBase>(UGameplayStatics::GetGameMode(WorldContextObject));
	if (SntpGameMode == nullptr)
	{
		return;
	}
	UCharacterClassInfo* ClassInfo = SntpGameMode->CharacterClassInfo;
	for (TSubclassOf<UGameplayAbility> AbilityClass : ClassInfo->CommonAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(AbilityClass, 1);
		ASC->GiveAbility(AbilitySpec);
	}
}

bool USntpAbilitySystemLibrary::IsCriticalHit(const FGameplayEffectContextHandle& EffectContextHandle)
{
	if (const FSntpGameplayEffectContext* SntpContext = static_cast<const FSntpGameplayEffectContext*>(EffectContextHandle.Get()))
	{
		return SntpContext->IsCritical();
	}
	return false;
}

FText USntpAbilitySystemLibrary::FormatTime(int32 TotalSeconds)
{
	int32 Hours = TotalSeconds / 3600;
	int32 Minutes = (TotalSeconds % 3600) / 60;
	int32 Seconds = TotalSeconds % 60;

	FNumberFormattingOptions Options;
	Options.MinimumIntegralDigits = 2;
	Options.MaximumIntegralDigits = 2;

	return FText::Format(
		NSLOCTEXT("Time", "HHMMSS", "{0}:{1}:{2}"),
		FText::AsNumber(Hours, &Options),
		FText::AsNumber(Minutes, &Options),
		FText::AsNumber(Seconds, &Options)
	);
}

AActor* USntpAbilitySystemLibrary::GetNearestEnemy(AActor* Source, float Radius)
{
	if (Source == nullptr) return nullptr;
	UWorld* World = Source->GetWorld();
	if (World == nullptr) return nullptr;
	
	TArray<AActor*> OverlappingActors;
	
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));
	
	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(Source);
	// Detect
	UKismetSystemLibrary::SphereOverlapActors(
		World,
		Source->GetActorLocation(),
		Radius,
		ObjectTypes,
		AActor::StaticClass(),
		IgnoreActors,
		OverlappingActors
		);
	AActor* NearestActor = nullptr;
	float MinDestSq = TNumericLimits<float>::Max();
	
	for (AActor* Actor : OverlappingActors)
	{
		if (!Actor) continue;
		if (!IsEnemy(Source, Actor)) continue;
		float DistSq = FVector::DistSquared(Source->GetActorLocation(), Actor->GetActorLocation());
		if (DistSq < MinDestSq)
		{
			MinDestSq = DistSq;
			NearestActor = Actor;
		}
	}
	return NearestActor;
}

bool USntpAbilitySystemLibrary::IsEnemy(AActor* Source, AActor* Target)
{
	if (!Source || !Target) return false;
	return Target->ActorHasTag(FName("Enemy"));
}

bool USntpAbilitySystemLibrary::IsSameInteractionOption(const FInteractionOption& A, const FInteractionOption& B)
{
	return (A.SourceActor == B.SourceActor && A.OptionName == B.OptionName);
}

FText USntpAbilitySystemLibrary::GetItemTypeFromTag(const FGameplayTag& Tag)
{
	FSntpGameplayTags SntpTags = FSntpGameplayTags::Get();
	if (Tag.MatchesTagExact(SntpTags.ItemType_Building))
	{
		return FText::FromString(TEXT("建筑"));
	}
	if (Tag.MatchesTagExact(SntpTags.ItemType_Consume))
	{
		return FText::FromString(TEXT("消耗品"));
	}
	if (Tag.MatchesTagExact(SntpTags.ItemType_Material))
	{
		return FText::FromString(TEXT("材料"));
	}
	if (Tag.MatchesTagExact(SntpTags.ItemType_Sell))
	{
		return FText::FromString(TEXT("售卖品"));
	}
	return FText::FromString(TEXT(""));
}

FLinearColor USntpAbilitySystemLibrary::GetLinearColorFromRarity(const FGameplayTag& Rarity)
{
	FSntpGameplayTags SntpTags = FSntpGameplayTags::Get();
	if (Rarity.MatchesTagExact(SntpTags.Rarity_White))
	{
		return FLinearColor(0.3f, 0.3f, 0.3f);
	}
	if (Rarity.MatchesTagExact(SntpTags.Rarity_Green))
	{
		return FLinearColor(0.01f, 0.3f, 0.01f);
	}
	if (Rarity.MatchesTagExact(SntpTags.Rarity_Blue))
	{
		return FLinearColor(0.f, 0.45f, 0.8f);
	}
	if (Rarity.MatchesTagExact(SntpTags.Rarity_Yellow))
	{
		return FLinearColor(0.7f, 0.55f, 0.0f);
	}
	return FLinearColor(1.0f, 1.0f, 1.0f);
}
