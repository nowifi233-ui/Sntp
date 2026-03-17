// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ZiplineNode.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "SntpGameplayTags.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

AZiplineNode::AZiplineNode()
{
	ConnectRadius = 8000.f;
}

void AZiplineNode::FindConnectedNodes()
{
	TArray<AActor*> Nodes;
	
	UGameplayStatics::GetAllActorsOfClass(
		GetWorld(),
		AZiplineNode::StaticClass(),
		Nodes
	);
	
	for (AActor* Actor : Nodes)
	{
		AZiplineNode* Node = Cast<AZiplineNode>(Actor);
		if (Node == this) continue;
		float Distance = FVector::Dist(GetActorLocation(), Node->GetActorLocation());
		if (Distance < ConnectRadius)
		{
			ConnectedNodes.Add(Node);
		}
	}
}

TArray<FInteractionOption> AZiplineNode::GetInteractionOptions()
{
	return Options;
}

void AZiplineNode::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == FName("Ride"))
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
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, GameplayTags.Damage, 20.f);
			InteractorASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}
	if (OptionName == FName("Heal"))
	{
		GEngine->AddOnScreenDebugMessage(0, 5, FColor::Red, FString::Printf(TEXT("Heal Interact")), false);
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
			UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(EffectSpecHandle, GameplayTags.Damage, -20.f);
			InteractorASC->ApplyGameplayEffectSpecToSelf(*EffectSpecHandle.Data.Get());
		}
	}
	if (OptionName == FName("Retrieve"))
	{
		Destroy();
	}
}

void AZiplineNode::BeginPlay()
{
	Super::BeginPlay();
	FindConnectedNodes();
}


