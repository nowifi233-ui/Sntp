// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/SnCompanion.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "Characters/SnCompanionAIController.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameplayEffect.h"

ASnCompanion::ASnCompanion()
{
	// ASC and AttributeSet as character subobjects (same pattern as the
	// enemy): the set is registered automatically, and
	// InitAbilityActorInfo(self, self) wires everything up.
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<USnAttributeSet>(TEXT("AttributeSet"));

	// Spawned companions automatically get the follow + shoot controller.
	AIControllerClass = ASnCompanionAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	// Walk through the player (and other pawns) instead of blocking them - the
	// companion must not stop the player or the chase gets stuck on the owner.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// The camera arm's probe must pass through the companion (same as the
	// enemy): it shortens only against real world geometry, so a companion
	// standing between the player and the camera never yanks the camera close.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
}

UAbilitySystemComponent* ASnCompanion::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

ASnCompanion* ASnCompanion::GetActiveCompanion(UObject* WorldContextObject, AActor* InOwner)
{
	if (!WorldContextObject || !InOwner)
	{
		return nullptr;
	}

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		for (TActorIterator<ASnCompanion> It(World); It; ++It)
		{
			if (It->GetOwner() == InOwner)
			{
				return *It;
			}
		}
	}
	return nullptr;
}

void ASnCompanion::BeginPlay()
{
	Super::BeginPlay();

	InitAbilityActorInfo();
	BindAttributeCallbacks();

	// The summon ability SetOwner()s us after spawn, so the owner is not
	// available yet at BeginPlay. The timer retries the owner-death bind
	// until the owner appears, then stops: despawning is the summon GA's job
	// (its EndAbility destroys its own companion on cancel / unequip).
	GetWorldTimerManager().SetTimer(OwnerBindTimerHandle, this, &ASnCompanion::HandleOwnerBindTimer, 0.1f, true);
}

void ASnCompanion::HandleOwnerBindTimer()
{
	// The summon ability SetOwner()s us after spawn, so the owner may not
	// exist yet - retry until the owner's death delegate is bound, then stop.
	// Despawning is managed by the summon GA: its EndAbility destroys its own
	// companion (cancel and unequip both route there), nothing left to poll.
	if (BindOwnerDeath())
	{
		GetWorldTimerManager().ClearTimer(OwnerBindTimerHandle);
	}
}

void ASnCompanion::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Unbind before destruction: Destroy() doesn't clear dynamic multicast
	// entries, and a stale entry on the owner's OnDeath would call into a dead
	// object.
	if (AttributeSet)
	{
		AttributeSet->OnDeath.RemoveDynamic(this, &ASnCompanion::HandleCompanionDeath);
	}
	if (USnAttributeSet* OwnerSet = GetOwnerAttributeSet())
	{
		OwnerSet->OnDeath.RemoveDynamic(this, &ASnCompanion::HandleOwnerDied);
	}
	Super::EndPlay(EndPlayReason);
}

void ASnCompanion::InitAbilityActorInfo()
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	// The attack skill: granted from its own slot (same configurable GA slot
	// type as the player's AttackAbilityClass). The AI controller activates it
	// by class (FindAbilitySpecFromClass), so a missing grant fails silently.
	if (AttackAbilityClass)
	{
		FGameplayAbilitySpec Spec(AttackAbilityClass, 1);
		Spec.SourceObject = this;
		AbilitySystemComponent->GiveAbility(Spec);
	}

	// Extra abilities (passives, etc.), granted the same way.
	for (const TSubclassOf<UGameplayAbility>& AbilityClass : GrantedAbilities)
	{
		if (AbilityClass)
		{
			FGameplayAbilitySpec Spec(AbilityClass, 1);
			Spec.SourceObject = this;
			AbilitySystemComponent->GiveAbility(Spec);
		}
	}

	if (DefaultAttributesEffect)
	{
		FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
		Context.AddSourceObject(this);
		const FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributesEffect, 1.f, Context);
		if (Spec.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
		}
	}
}

void ASnCompanion::BindAttributeCallbacks()
{
	if (AttributeSet)
	{
		AttributeSet->OnDeath.RemoveDynamic(this, &ASnCompanion::HandleCompanionDeath);
		AttributeSet->OnDeath.AddDynamic(this, &ASnCompanion::HandleCompanionDeath);
	}
}

bool ASnCompanion::BindOwnerDeath()
{
	if (bOwnerBound)
	{
		return true;
	}

	// The player's ASC lives on the PlayerState; the pawn's
	// GetAbilitySystemComponent() forwards to it, so the helper works on the
	// owner pawn directly.
	if (USnAttributeSet* OwnerSet = GetOwnerAttributeSet())
	{
		OwnerSet->OnDeath.RemoveDynamic(this, &ASnCompanion::HandleOwnerDied);
		OwnerSet->OnDeath.AddDynamic(this, &ASnCompanion::HandleOwnerDied);
		bOwnerBound = true;
		return true;
	}
	return false;
}

USnAttributeSet* ASnCompanion::GetOwnerAttributeSet() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	if (UAbilitySystemComponent* OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor))
	{
		// GetAttributeSet returns a const set in 5.7; we only bind delegates
		// (and unbind the same ones later), so the cast is safe.
		return const_cast<USnAttributeSet*>(Cast<const USnAttributeSet>(
			OwnerASC->GetAttributeSet(USnAttributeSet::StaticClass())));
	}
	return nullptr;
}

void ASnCompanion::HandleCompanionDeath(AActor* Killer)
{
	// Killed in combat: no death sequence, no loot - just despawn. Server-side
	// only; clients see the replicated despawn.
	if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
	{
		Destroy();
	}
}

void ASnCompanion::HandleOwnerDied(AActor* InInstigator)
{
	if (GetWorld() && GetWorld()->GetNetMode() != NM_Client)
	{
		Destroy();
	}
}
