// Fill out your copyright notice in the Description page of Project Settings.

#include "Characters/SnEnemyAIController.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Characters/SnEnemy.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Navigation/PathFollowingComponent.h"

void ASnEnemyAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// The controller can briefly exist without a pawn (spawn order, or the
	// enemy is mid-destroy) - don't touch a null pawn.
	if (!GetPawn())
	{
		return;
	}

	// Mid-swing: the enemy planted its feet when the swing started
	// (ASnEnemy::Tick stops the movement) and holds that pose for the whole
	// animation. Re-issuing a move request now would drag it forward through
	// its own attack - so the chase is simply skipped until the swing ends.
	// Target bookkeeping resumes with it (a swing is short; a target that fled
	// far enough is dropped on the next tick).
	if (const ASnEnemy* Enemy = Cast<ASnEnemy>(GetPawn()))
	{
		if (Enemy->IsAttacking())
		{
			return;
		}
	}

	// Drop the target when it's gone or wandered beyond the lose radius
	// (hysteresis: larger than DetectionRadius so the chase doesn't flicker
	// on and off while the target hovers on the border).
	if (!CurrentTarget.IsValid()
		|| FVector::Dist2D(GetPawn()->GetActorLocation(), CurrentTarget->GetActorLocation()) > LoseRadius)
	{
		CurrentTarget = nullptr;
	}

	if (!CurrentTarget.IsValid())
	{
		// Idle: re-scan for a tagged actor on an interval, not every frame.
		const float Now = GetWorld()->GetTimeSeconds();
		if (Now - LastScanTime < RescanInterval)
		{
			return;
		}
		LastScanTime = Now;

		AActor* Nearest = FindNearestTarget();
		if (!Nearest)
		{
			return;
		}
		CurrentTarget = Nearest;
	}

	// Re-path only when the previous move finished (or never started), so we
	// don't spam MoveToActor every frame.
	if (GetMoveStatus() != EPathFollowingStatus::Type::Moving)
	{
		MoveToActor(CurrentTarget.Get(), AcceptanceRadius, /*bStopOnOverlap=*/true);
	}
}

AActor* ASnEnemyAIController::FindNearestTarget() const
{
	AActor* Nearest = nullptr;
	float NearestDist2D = DetectionRadius; // squared to be safe against scans

	// Actor tags, not gameplay tags: fast, replicated, and readable from BP
	// (the tag shows up in the level as a plain Actor Tag on the player BP).
	TArray<AActor*> Tagged;
	UGameplayStatics::GetAllActorsWithTag(GetWorld(), SnGameplayTags::Actor_Player, Tagged);
	for (AActor* Candidate : Tagged)
	{
		const float Dist2D = FVector::Dist2D(GetPawn()->GetActorLocation(), Candidate->GetActorLocation());
		if (Dist2D <= DetectionRadius && Dist2D < NearestDist2D)
		{
			Nearest = Candidate;
			NearestDist2D = Dist2D;
		}
	}
	return Nearest;
}
