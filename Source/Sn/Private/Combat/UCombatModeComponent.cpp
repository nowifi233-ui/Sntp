// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/UCombatModeComponent.h"
#include "Characters/SnEnemy.h"
#include "Engine/OverlapResult.h"
#include "TimerManager.h"

UCombatModeComponent::UCombatModeComponent()
{
	// Timer-driven scan; no per-frame tick needed.
	PrimaryComponentTick.bCanEverTick = false;
}

void UCombatModeComponent::BeginPlay()
{
	Super::BeginPlay();
	StartScanning();
}

void UCombatModeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ScanTimerHandle.IsValid() && GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ScanTimerHandle);
	}

	// Never leave health bars dangling when the component goes away.
	for (ASnEnemy* Enemy : EnemiesInCombat)
	{
		if (IsValid(Enemy))
		{
			Enemy->SetHealthBarVisible(false);
		}
	}
	EnemiesInCombat.Reset();

	Super::EndPlay(EndPlayReason);
}

void UCombatModeComponent::StartScanning()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(ScanTimerHandle, this, &UCombatModeComponent::Scan, ScanInterval, /*bLoop=*/true);
	}
	Scan();
}

void UCombatModeComponent::Scan()
{
	const AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return;
	}

	// Overlap everything in range, keep the enemies.
	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(SnCombatModeScan), /*bTraceComplex=*/false);
	QueryParams.AddIgnoredActor(Owner);
	World->OverlapMultiByObjectType(
		Overlaps,
		Owner->GetActorLocation(),
		FQuat::Identity,
		FCollisionObjectQueryParams(FCollisionObjectQueryParams::InitType::AllObjects),
		FCollisionShape::MakeSphere(CombatRadius),
		QueryParams);

	TArray<ASnEnemy*> Found;
	for (const FOverlapResult& Result : Overlaps)
	{
		if (ASnEnemy* Enemy = Cast<ASnEnemy>(Result.GetActor()))
		{
			if (!Found.Contains(Enemy))
			{
				Found.Add(Enemy);
			}
		}
	}

	// Diff: show bars for enemies entering the radius, hide for those leaving.
	for (ASnEnemy* Enemy : Found)
	{
		if (!EnemiesInCombat.Contains(Enemy))
		{
			Enemy->SetHealthBarVisible(true);
		}
	}
	for (ASnEnemy* Enemy : EnemiesInCombat)
	{
		if (IsValid(Enemy) && !Found.Contains(Enemy))
		{
			Enemy->SetHealthBarVisible(false);
		}
	}

	EnemiesInCombat = Found;

	const bool bNewState = EnemiesInCombat.Num() > 0;
	if (bNewState != bInCombat)
	{
		bInCombat = bNewState;
		OnCombatModeChanged.Broadcast(bInCombat);
	}
}
