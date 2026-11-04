// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/ASnBuildPreview.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Game/SnSettings.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

ASnBuildPreview::ASnBuildPreview()
{
	PrimaryActorTick.bCanEverTick = false;

	// Local only: each machine spawns and drives its own ghost. Leaving
	// replication on would push a preview into other players' worlds for no
	// benefit - it is a picture of MY aim, not a world object.
	SetReplicates(false);

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(MeshComponent);

	// No collision whatsoever: the ghost must never block the placement trace
	// that computes where it goes (a ghost blocking its own aim would freeze
	// half a meter in front of the player), shove pawns, or catch attacks.
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComponent->SetGenerateOverlapEvents(false);
}

void ASnBuildPreview::SetPreviewMesh(UStaticMesh* Mesh, UMaterialInterface* GhostMaterial)
{
	if (!Mesh)
	{
		return;
	}

	// SetStaticMesh resets every material slot to the mesh's own materials, so
	// the tint has to come after it - and re-applied on every swap, since a
	// new mesh means new slots.
	MeshComponent->SetStaticMesh(Mesh);

	if (!GhostMaterial)
	{
		return;
	}

	if (!GhostMID)
	{
		GhostMID = UMaterialInstanceDynamic::Create(GhostMaterial, this);
	}

	for (int32 MaterialIndex = 0; MaterialIndex < MeshComponent->GetNumMaterials(); ++MaterialIndex)
	{
		MeshComponent->SetMaterial(MaterialIndex, GhostMID);
	}

	// Fresh MID with no "Color" written: the next SetPlacementValid only writes
	// when the answer changed, so the first tint has to happen here or a ghost
	// that starts out INVALID would stay the material's default color until the
	// player happened to move somewhere legal first.
	ApplyTint();
}

void ASnBuildPreview::SetPlacementValid(bool bValid)
{
	// The per-frame caller passes the same answer most frames (the spot only
	// changes when the player moves or looks elsewhere) - writing the parameter
	// again would push the same value into the material every frame.
	if (bValid == bPreviewValid)
	{
		return;
	}
	bPreviewValid = bValid;
	ApplyTint();
}

void ASnBuildPreview::ApplyTint()
{
	if (!GhostMID)
	{
		// No ghost material configured: the mesh keeps its own materials and the
		// answer simply cannot be shown as a color (placement still works).
		return;
	}

	GhostMID->SetVectorParameterValue(TEXT("Color"), bPreviewValid
		? USnSettings::Get()->GetBuildPreviewValidColor()
		: USnSettings::Get()->GetBuildPreviewInvalidColor());
}
