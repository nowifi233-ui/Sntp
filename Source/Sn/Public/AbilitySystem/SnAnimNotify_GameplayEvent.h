// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "SnAnimNotify_GameplayEvent.generated.h"

/**
 * Sends a gameplay event to the mesh owner's ability system at this notify's
 * frame. The engine ships no notify that does this (UAnimNotify_GameplayCue
 * only fires cues), so an attack montage needs this bridge to hand control
 * back to the ability mid-animation: place it on the montage's hit frame with
 * Event.AttackWindow, and the listening ability
 * (USnGA_SphereAttack::OnAttackWindowEvent - shared by every montage role)
 * resolves the damage there instead of at activation - the swing lands when
 * the weapon does, not when the button went down.
 *
 * Dispatch is local: every machine playing the montage sends the event into
 * its own copy of the avatar's ASC, no RPC involved (each side's own montage
 * drives its own timeline - the client does not wait for the server's). What
 * that machine then does with the event is the listener's decision: the attack
 * window runs on the authority only.
 */
UCLASS(meta = (DisplayName = "Sn Gameplay Event"))
class SN_API USnAnimNotify_GameplayEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	/** Show the tag on the montage's notify track: a multi-notify montage stays
	 *  readable at a glance, and a tag no listener is waiting on (silent
	 *  zero-damage swings) is visible instead of hidden behind a generic name. */
	virtual FString GetNotifyName_Implementation() const override;

	/** Event tag sent to the mesh owner (e.g. Event.AttackWindow). Invalid tag =
	 *  nothing is sent. */
	UPROPERTY(EditAnywhere, Category = "Sn")
	FGameplayTag EventTag;
};
