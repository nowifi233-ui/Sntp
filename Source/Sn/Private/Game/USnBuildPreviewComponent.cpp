// Fill out your copyright notice in the Description page of Project Settings.

#include "Game/USnBuildPreviewComponent.h"

#include "AbilitySystem/SnGA_Build.h"
#include "AbilitySystemComponent.h"
#include "Game/ASnBuildPreview.h"
#include "Game/SnSettings.h"
#include "Game/Save/SnSaveTypes.h"
#include "Game/USnBuildLibrary.h"
#include "GameFramework/Pawn.h"
#include "Inventory/FSnItemTypes.h"
#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnItemLibrary.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"

USnBuildPreviewComponent::USnBuildPreviewComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void USnBuildPreviewComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// The ghost is a separate actor - leaving the level does not take it along.
	if (GhostActor)
	{
		GhostActor->Destroy();
		GhostActor = nullptr;
	}
	GhostDeviceID = NAME_None;

	Super::EndPlay(EndPlayReason);
}

const USnInventoryComponent* USnBuildPreviewComponent::GetInventoryComponent() const
{
	const APawn* Pawn = Cast<APawn>(GetOwner());
	if (!Pawn)
	{
		return nullptr;
	}

	// The player state's copy is the one that exists in every path (the pawn
	// carries no inventory of its own); the fallback covers a pawn that was
	// handed one directly, which is only how the non-player test actors do it.
	if (const ASnPlayerState* SnPS = Pawn->GetPlayerState<ASnPlayerState>())
	{
		if (const USnInventoryComponent* Inventory = SnPS->FindComponentByClass<USnInventoryComponent>())
		{
			return Inventory;
		}
	}
	return Pawn->FindComponentByClass<USnInventoryComponent>();
}

const USnGA_Build* USnBuildPreviewComponent::FindSelectedBuildAbility() const
{
	const USnInventoryComponent* Inventory = GetInventoryComponent();
	const FName RowName = Inventory ? Inventory->GetSelectedHotbarRowName() : NAME_None;
	const FSnItemRow* Row = USnItemLibrary::FindItemRow(RowName);
	if (!Row || !Row->UseAbility)
	{
		return nullptr;
	}

	// The granted spec is what "held" means (USnInventoryComponent::
	// RefreshHotbarGrant grants the selected slot's UseAbility on the server,
	// and a granted spec replicates to its owner). Reading it here rather than
	// reading the row's UseAbility alone is the point: this machine only draws a
	// ghost for an ability it actually HAS.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	ASnPlayerState* SnPS = Pawn ? Pawn->GetPlayerState<ASnPlayerState>() : nullptr;
	UAbilitySystemComponent* ASC = SnPS ? SnPS->GetAbilitySystemComponent() : nullptr;
	if (!ASC)
	{
		return nullptr;
	}

	for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		// Both conditions, one fact: the granted ability IS the one this row
		// would use, rather than merely *a* build ability. On a client the
		// selection and the grant arrive independently, so the frame or two in
		// which they disagree draw nothing - holding a kit is one fact, and it
		// has to be the same fact on both streams.
		if (Spec.Ability && Spec.Ability->GetClass() == Row->UseAbility.Get())
		{
			if (const USnGA_Build* Build = Cast<USnGA_Build>(Spec.Ability.Get()))
			{
				return Build;
			}
		}
	}
	return nullptr;
}

void USnBuildPreviewComponent::HideGhost()
{
	if (GhostActor)
	{
		GhostActor->SetActorHiddenInGame(true);
	}
}

void USnBuildPreviewComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Ghosts are for the player doing the building; an AI-controlled or remote
	// copy of this pawn on the same machine (a listen server's client pawn)
	// must not show one. This runs first every frame, which is what makes the
	// always-on tick cheap.
	const APawn* Pawn = Cast<APawn>(GetOwner());
	// ASnPlayerController, not APlayerController: the window state (and later
	// anything else UI-side) lives on the project's controller subclass.
	ASnPlayerController* PC = Pawn ? Cast<ASnPlayerController>(Pawn->GetController()) : nullptr;
	const USnInventoryComponent* Inventory = GetInventoryComponent();
	if (!Pawn || !Inventory || !Pawn->IsLocallyControlled() || !PC)
	{
		HideGhost();
		return;
	}

	const USnGA_Build* BuildAbility = FindSelectedBuildAbility();
	const FName RowName = Inventory->GetSelectedHotbarRowName();

	// The device's own row carries the preview mesh; without one there is
	// nothing to draw (placement still works - the mesh is presentation only),
	// so this is one more reason to show no ghost.
	const FSnBuildDeviceRow* DeviceRow = BuildAbility
		? USnBuildLibrary::FindBuildDeviceRow(BuildAbility->GetDeviceID())
		: nullptr;

	// A window is open: input is UI-only (USnWindowManager), so the use input
	// cannot reach the item and the gameplay behind the window is paused in
	// standalone. Showing "you could build here" through the window would be a
	// promise the input cannot keep.
	bool bCanShow = BuildAbility != nullptr
		&& DeviceRow != nullptr && DeviceRow->PreviewMesh != nullptr
		&& !PC->IsConfigWindowOpen();

	// The same gates the server applies on a use, in the same order: the bag has
	// to still hold the item (placing the last one consumes it) and the cooldown
	// has to be ready. Whatever the ghost shows is what a use would do right now.
	if (bCanShow)
	{
		bCanShow = Inventory->HasItem(RowName, 1)
			&& Inventory->GetLocalUseCooldownRemaining(RowName) <= 0.f;
	}

	FVector Location;
	FRotator Rotation;
	bool bValid = false;
	if (bCanShow)
	{
		// Same geometry as the ability: the ghost is where the device would land,
		// not an approximation of it. No ground under the aim (looking off the
		// map) means there is nowhere to build and the ghost goes away too.
		bCanShow = USnBuildLibrary::ComputePlacementPoint(Pawn, PC,
			BuildAbility->GetPlacementRange(), Location, Rotation);
		if (bCanShow)
		{
			bValid = USnBuildLibrary::IsPlacementValid(Pawn, Location, BuildAbility->GetMinDistance());
		}
	}

	if (!bCanShow)
	{
		// Nothing to draw (nothing held, item gone, cooldown running, window
		// open, aiming at empty sky): hide rather than destroy - the next frame
		// may want the ghost again.
		HideGhost();
		return;
	}

	if (!GhostActor && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		// No owner and no instigator: the ghost is nobody's actor - it must not
		// turn up in damage attribution, AI targeting or anything else that
		// walks the ownership chain.
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		GhostActor = GetWorld()->SpawnActor<ASnBuildPreview>(
			ASnBuildPreview::StaticClass(), Location, Rotation, SpawnParams);
	}
	if (!GhostActor)
	{
		return;
	}

	if (GhostDeviceID != BuildAbility->GetDeviceID())
	{
		// Swap the mesh on the existing ghost (one ghost per machine, rebuilt
		// only when the held ability moves to another device).
		GhostActor->SetPreviewMesh(DeviceRow->PreviewMesh, USnSettings::Get()->GetBuildPreviewMaterial());
		GhostDeviceID = BuildAbility->GetDeviceID();
	}

	// Tint before the transform: one move, and the color is already right when
	// the ghost lands in its new spot.
	GhostActor->SetPlacementValid(bValid);
	GhostActor->SetActorLocationAndRotation(Location, Rotation);
	GhostActor->SetActorHiddenInGame(false);
}
