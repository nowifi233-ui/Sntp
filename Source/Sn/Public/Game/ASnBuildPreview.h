// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ASnBuildPreview.generated.h"

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UStaticMesh;
class UStaticMeshComponent;

/**
 * The transparent stand-in shown where a device would land. Purely local
 * presentation: one per machine, spawned and driven by that machine's
 * USnBuildPreviewComponent, never replicated - it carries no state anyone else
 * needs, and the placement itself stays server-authoritative in USnGA_Build.
 *
 * It also never instantiates the device class it previews: the mesh comes from
 * the build-device row, so a ghost costs one static mesh and cannot run device
 * logic (or collide with anything) by accident.
 */
UCLASS()
class SN_API ASnBuildPreview : public AActor
{
	GENERATED_BODY()

public:
	ASnBuildPreview();

	/** Shows Mesh for this device, tinted with GhostMaterial ("Color" vector
	 *  parameter). Called on every device switch - the mesh is swapped on the
	 *  existing actor, so a ghost is spawned once and then reused. A null mesh
	 *  or material simply leaves that part alone (no material configured = the
	 *  device's own materials, untinted). */
	void SetPreviewMesh(UStaticMesh* Mesh, UMaterialInterface* GhostMaterial);

	/** Tints the ghost green or red. No-op when the state did not change, so
	 *  the per-frame caller can call it unconditionally. */
	void SetPlacementValid(bool bValid);

private:
	/** Writes the current state into the ghost material's "Color" parameter
	 *  (no-op without one). */
	void ApplyTint();

	/** The ghost body - a bare mesh, no collision at all: the preview must
	 *  never block the trace that computes where it goes, and it must not
	 *  shove pawns around. */
	UPROPERTY(VisibleAnywhere, Category = "BuildPreview")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** Translucent instance of the Sn Settings ghost material; created on the
	 *  first SetPreviewMesh and reused across device switches. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> GhostMID;

	/** Last tint written - the diff key for SetPlacementValid. */
	bool bPreviewValid = true;
};
