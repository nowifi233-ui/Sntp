// Fill out your copyright notice in the Description page of Project Settings.

#include "AbilitySystem/SnGA_Build.h"

#include "Engine/World.h"
#include "Game/Save/SnSaveTypes.h"
#include "Game/USnBuildLibrary.h"
#include "Game/USnGameInstance.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

USnGA_Build::USnGA_Build()
{
	// See the class doc: placement is server-authoritative, so this must never run
	// locally on a client. ServerOnly is also the only policy under which the
	// one-shot path's grant -> activate -> remove lifecycle completes inside one
	// server frame (an empty prediction key cannot ride the ServerInitiated
	// client push either).
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
	// Fresh instance per use: the placement carries no state between uses, and no
	// lingering instance can block the next activation - which the held window
	// leans on, since its spec stays granted for as long as the item is the
	// selected slot.
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
}

bool USnGA_Build::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// This override runs on the CDO, never on an instance: the ASC passes the
	// spec's Ability object, an InstancedPerExecution spec has no instance yet,
	// and the base class comments on the same (GameplayAbility.cpp:459). So
	// everything must come from the ActorInfo PARAMETER - the
	// GetAvatarActorFromActorInfo() family reads CurrentActorInfo, which is unset
	// here: it returns null AND trips its own ensure.
	AActor* Avatar = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
	if (!Avatar)
	{
		return false;
	}

	// A device row that cannot spawn anything (missing row, or a row with no
	// class) would consume the item and place nothing - reject it up front.
	const FSnBuildDeviceRow* Row = USnBuildLibrary::FindBuildDeviceRow(DeviceID);
	if (!Row || !Row->DeviceClass)
	{
		return false;
	}

	// The placement check belongs here and not in ActivateAbility: this is what
	// makes a bad spot free. Either entry path returns before the cooldown starts
	// or the item is consumed - the one-shot path because
	// GiveAbilityAndActivateOnce clears the spec and returns an INVALID handle
	// when activation fails (AbilitySystemComponent_Abilities.cpp:366-372), the
	// held path because TryActivateAbility returns false and leaves the granted
	// spec alone. Both are checked in USnInventoryComponent::ServerUseItem, so
	// "nothing happened" falls out of the engine contract with no rollback code
	// of our own.
	FVector Location;
	FRotator Rotation;
	if (!USnBuildLibrary::ComputePlacementPoint(Cast<APawn>(Avatar), ActorInfo->PlayerController.Get(), PlacementRange, Location, Rotation))
	{
		return false;
	}
	return USnBuildLibrary::IsPlacementValid(Avatar, Location, MinDistance);
}

void USnGA_Build::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UWorld* World = Avatar ? Avatar->GetWorld() : nullptr;
	USnGameInstance* GameInstance = World ? World->GetGameInstance<USnGameInstance>() : nullptr;
	APawn* Pawn = Cast<APawn>(Avatar);
	APlayerController* PC = ActorInfo ? ActorInfo->PlayerController.Get() : nullptr;

	// CanActivateAbility validated this same spot a few engine calls ago, in this
	// same frame, so this is a re-run of the same pure function on the same
	// inputs. It is kept anyway: placing a device at a location we could not
	// compute is worse than placing none at all.
	FVector Location;
	FRotator Rotation;
	if (!GameInstance || !Pawn
		|| !USnBuildLibrary::ComputePlacementPoint(Pawn, PC, PlacementRange, Location, Rotation)
		|| !USnBuildLibrary::IsPlacementValid(Avatar, Location, MinDistance))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/true);
		return;
	}

	// Server-authoritative: the record goes into the snapshot (so this level
	// rebuilds the device on its next load) and the actor is spawned here. The
	// spawn replicates - every client sees the device appear.
	GameInstance->AddBuiltDevice(DeviceID, Location, Rotation);

	EndAbility(Handle, ActorInfo, ActivationInfo, /*bReplicateEndAbility=*/false, /*bWasCancelled=*/false);
}

FText USnGA_Build::GetDescription_Implementation(int32 Level)
{
	// Nothing configured: the base text (usually empty, which drops the line from
	// the tooltip entirely) beats announcing that the ability places "None".
	if (DeviceID.IsNone())
	{
		return Super::GetDescription_Implementation(Level);
	}

	// The row is where a device's name lives - a renamed device can never leave
	// an item describing something the table no longer has. A missing row or an
	// empty display name shows the raw row name instead of vanishing:
	// misconfiguration has to be visible, the same convention the item
	// description placeholders follow.
	const FSnBuildDeviceRow* Row = USnBuildLibrary::FindBuildDeviceRow(DeviceID);
	const FText DeviceName = (Row && !Row->DisplayName.IsEmpty()) ? Row->DisplayName : FText::FromName(DeviceID);

	// Style tag on the device name, the same rich-text convention the item stats
	// use (<DT_TextStyles row>text</>); no row of that name yet just renders the
	// text in the Default style, which is the intended "not styled yet" look
	// rather than an error. See DESCRIPTION_TEMPLATE.md.
	const FText StyledName = FText::Format(FText::FromString(TEXT("<Build.Device>{0}</>")), DeviceName);

	// One sentence of game copy, the same shape the damage family uses (and the
	// item side prefixes it with the ability's catalog name). Level is unused on
	// purpose: a device is the same device at every level.
	return FText::Format(FText::FromString(TEXT("在视线前方放置{0}")), StyledName);
}
