// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/SnWidgetController.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "Player/SnPlayerState.h"

void USnWidgetController::Initialize(APlayerController* InPC)
{
	PlayerController = InPC;
	PlayerState = InPC ? InPC->PlayerState : nullptr;

	if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(PlayerState))
	{
		AbilitySystemComponent = ASCInterface->GetAbilitySystemComponent();
	}

	if (AbilitySystemComponent)
	{
		// Take the attribute set from the PlayerState member (Aura pattern)
		// rather than GetSet<T>() (which searches the SpawnedAttributes
		// replication array): on a networked client this controller is created
		// from HUD BeginPlay (OnRep_ReplicatedHasBegunPlay), which can run
		// before the ASC's SpawnedAttributes replicate - GetSet would return
		// null and the cached null would leave the UI blank forever. The
		// PlayerState subobject exists on every client from construction
		// (values start at 0 and refresh via the attribute change delegates
		// once replication lands).
		if (const ASnPlayerState* SnPS = Cast<ASnPlayerState>(PlayerState))
		{
			AttributeSet = SnPS->GetAttributeSet();
		}
		// Fallback for exotic setups (no player state owning the set).
		if (!AttributeSet)
		{
			// GetSet<T>() returns const T*; the set is owned by the ASC, we only hold a reference.
			AttributeSet = const_cast<USnAttributeSet*>(AbilitySystemComponent->GetSet<USnAttributeSet>());
		}
	}

	BindCallbacks();
}
