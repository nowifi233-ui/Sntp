// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SnTutorialZone.generated.h"

class USphereComponent;

/**
 * Place-in-level ONE-TIME tutorial trigger - the same dialogue popup the hub
 * intro uses, driven by the same DT_Dialogue script. Drop one on a path the
 * player must walk; the first time a real player enters the sphere, the
 * dialogue at TutorialRow plays (click to advance, Escape skips) and the zone
 * disables itself permanently: the milestone IS the tutorial, so it never
 * re-plays on later visits, re-entries to the level, or a loaded save.
 *
 * Single-player discipline, same as the hub intro tutorial: never fires in a
 * LAN session (the popup locks game input). The sphere overlaps any Pawn but
 * only IsPlayerControlled pawns act - AI-driven companions (which carry the
 * "Player" aggro tag) walk through without triggering.
 */
UCLASS()
class SN_API ASnTutorialZone : public AActor
{
	GENERATED_BODY()

public:
	ASnTutorialZone();

	/** Radius of the trigger sphere. Edits re-shape the sphere immediately. */
	UPROPERTY(EditAnywhere, Category = "Tutorial", meta = (ClampMin = "0.0"))
	float TriggerRadius = 300.f;

	/** The tutorial script to play: first row's name in the global dialogue
	 *  table (DT_Dialogue) - doubles as the milestone ID (write-once,
	 *  persisted with the save). See the dialogue section in CLAUDE.md. */
	UPROPERTY(EditAnywhere, Category = "Tutorial")
	FName TutorialRow;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;

private:
	UFUNCTION()
	void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** Applies TriggerRadius to the sphere (spawn + editor edits). */
	void ApplyRadius();

	UPROPERTY(VisibleAnywhere, Category = "Tutorial")
	TObjectPtr<USphereComponent> TriggerSphere;

#if WITH_EDITOR
	// Live radius preview: a native actor placed in a level does not
	// reconstruct on property edits, so resize the sphere by hand.
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
};
