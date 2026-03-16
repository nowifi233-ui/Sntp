// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ZiplineNode.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "SntpGameplayTags.h"
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

TArray<FInteractionOption> AZiplineNode::GetInteractionOptions(AActor* Interactor)
{
	TArray<FInteractionOption> Options;
	Options.Add({FName("Ride"), FText::FromString("Ride Zipline")});
	Options.Add({FName("Retrieve"), FText::FromString("Retrieve Zipline")});
	return Options;
}

void AZiplineNode::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == FName("Ride"))
	{
		IAbilitySystemInterface* ASCInt = Cast<IAbilitySystemInterface>(Interactor);
		if (ASCInt)
		{
			FSntpGameplayTags SntpTags = FSntpGameplayTags::Get();
			FGameplayTagContainer Container;
			Container.AddTag(SntpTags.Interact);
			ASCInt->GetAbilitySystemComponent()->TryActivateAbilitiesByTag(Container);
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


