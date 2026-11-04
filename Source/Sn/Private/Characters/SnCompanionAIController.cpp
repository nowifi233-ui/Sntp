// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/SnCompanionAIController.h"
#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/SnCompanion.h"
#include "Engine/World.h"
#include "Combat/USnCombatLibrary.h"
#include "Navigation/PathFollowingComponent.h"

void ASnCompanionAIController::BeginPlay()
{
	Super::BeginPlay();

	// Poll for shoot targets on a timer instead of every frame: shooting is a
	// 0.4s-latency decision, nobody can tell the difference.
	GetWorldTimerManager().SetTimer(ShootTimerHandle, this, &ASnCompanionAIController::TryShoot, ShootInterval, true);
}

void ASnCompanionAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// Following is a server-side decision (clients replicate the movement).
	if (GetWorld() && GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	AActor* OwnerActor = MyPawn->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Re-path only when the current move finished (or never started), so we
	// don't spam MoveToActor every frame - same throttle as the enemy chase.
	// Only chase when actually out of range; the companion lags behind at its
	// own speed instead of being glued to the owner.
	if (GetMoveStatus() != EPathFollowingStatus::Type::Moving
		&& FVector::Dist2D(MyPawn->GetActorLocation(), OwnerActor->GetActorLocation()) > FollowDistance)
	{
		MoveToActor(OwnerActor, AcceptanceRadius, /*bStopOnOverlap=*/true);
	}
}

APawn* ASnCompanionAIController::GetShootTarget_Implementation() const
{
	APawn* MyPawn = GetPawn();
	if (!MyPawn || !GetWorld())
	{
		return nullptr;
	}

	// One shared query: overlap + enemy-tag filter + nearest sort live in
	// USnCombatLibrary (blueprint ability GAs use the same node).
	return Cast<APawn>(USnCombatLibrary::FindNearestEnemy(GetWorld(), MyPawn->GetActorLocation(), ShootRadius));
}

void ASnCompanionAIController::TryShoot()
{
	if (!GetWorld() || GetWorld()->GetNetMode() == NM_Client)
	{
		return;
	}

	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	// No target, no shot. The ability itself is on the cooldown GAS already
	// knows about, so a shooting cooldown in progress simply fails silently -
	// TryActivateAbility returns false and nothing happens.
	if (!GetShootTarget())
	{
		return;
	}

	ASnCompanion* Companion = Cast<ASnCompanion>(MyPawn);
	if (!Companion)
	{
		return;
	}

	// Activate exactly the configured attack skill (same GA slot type as the
	// player's - skill-family assets). Granted once in
	// ASnCompanion::InitAbilityActorInfo; a missing grant or a cooldown in
	// progress fails silently.
	if (UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(MyPawn))
	{
		if (TSubclassOf<UGameplayAbility> SkillClass = Companion->GetAttackAbilityClass())
		{
			if (FGameplayAbilitySpec* Spec = ASC->FindAbilitySpecFromClass(SkillClass))
			{
				ASC->TryActivateAbility(Spec->Handle);
			}
		}
	}
}
