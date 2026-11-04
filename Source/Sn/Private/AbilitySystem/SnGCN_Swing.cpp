// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGCN_Swing.h"

#include "Camera/PlayerCameraManager.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

bool USnGCN_Swing::OnExecute_Implementation(AActor* Target, const FGameplayCueParameters& Parameters) const
{
	// The burst effects (swing arc + whoosh) are data-driven on the asset -
	// play them first, hit or miss, on every side that sees the cue.
	const bool bSpawnedAnyEffects = Super::OnExecute_Implementation(Target, Parameters);

	// Hit camera shake: only when the swing hit something (RawMagnitude = 1,
	// sent by ExecuteSphereAttack) and only on the attacker's own machine
	// (there the cue's instigator is a locally controlled pawn - observers
	// and the server never shake for a remote swing). A miss plays the arc
	// but never shakes.
	if (Parameters.RawMagnitude > 0.f && HitShakeClass)
	{
		AActor* Attacker = Parameters.Instigator.Get();
		if (APawn* AttackerPawn = Cast<APawn>(Attacker))
		{
			if (AttackerPawn->IsLocallyControlled())
			{
				if (APlayerController* PC = AttackerPawn->GetController<APlayerController>())
				{
					if (PC->PlayerCameraManager)
					{
						PC->PlayerCameraManager->StartCameraShake(HitShakeClass, HitShakeScale);
					}
				}
			}
		}
	}

	return bSpawnedAnyEffects;
}
