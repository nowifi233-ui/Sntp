// Fill out your copyright notice in the Description page of Project Settings.


#include "Game/SnGameState.h"

#include "Game/USnDifficultyLibrary.h"
#include "Net/UnrealNetwork.h"

void ASnGameState::SetDifficulty(FName InDifficulty)
{
	if (!HasAuthority())
	{
		return;
	}

	// Reject unknown tiers so a stale row name can never wedge the session
	// into a multiplier-less state (GetDifficultyMultiplier falls back to 1.0
	// anyway, but the menu should never show a tier the table does not know).
	const FSnDifficultyRow* Row = USnDifficultyLibrary::FindDifficultyRow(InDifficulty);
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("ASnGameState::SetDifficulty: unknown tier '%s' (no row in the difficulty table) - ignored"), *InDifficulty.ToString());
		return;
	}

	Difficulty = InDifficulty;
	// Server-local broadcast (OnRep never fires on the server - the listen
	// host's own menu reads the change through this, remote clients through
	// OnRep_Difficulty).
	OnDifficultyChanged.Broadcast(Difficulty);
}

float ASnGameState::GetDifficultyMultiplier() const
{
	const FSnDifficultyRow* Row = USnDifficultyLibrary::FindDifficultyRow(Difficulty);
	return Row ? Row->EnemyMultiplier : 1.f;
}

void ASnGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASnGameState, Difficulty);
}

void ASnGameState::OnRep_Difficulty()
{
	OnDifficultyChanged.Broadcast(Difficulty);
}
