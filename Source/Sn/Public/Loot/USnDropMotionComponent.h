// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "USnDropMotionComponent.generated.h"

class FLifetimeProperty;

/**
 * Ballistic drop animation shared by world pickups (item pickups and XP
 * shards): launches the owner on an upward impulse with a random sideways
 * scatter, simulates the arc in code (gravity + ground probe, no rigid body),
 * bounces softly once, then settles into a float animation (spin + bob).
 * One copy of the drop logic; every pickup type mounts it and tunes the
 * parameters per type in the BP.
 *
 * Call LaunchDrop() from BeginPlay (or later, to re-drop). IsSettled() tells
 * gameplay logic (e.g. XP magnetism) when the arc is over.
 *
 * Replication: the server rolls the launch point AND the initial velocity and
 * replicates both (DropStartLocation + DropInitialVelocity); every client
 * replays the SAME arc from the SAME start locally, so the visual motion never
 * depends on the server's replicated transform (5.7 push-model replication of
 * manual moves is unreliable for per-frame motion). The start point rides along
 * because a POOLED pickup is teleported to its new drop spot server-side, and
 * that move does not replicate: without it the client copy would launch from
 * the previous drop's landing spot and settle somewhere else entirely.
 */
UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class SN_API USnDropMotionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USnDropMotionComponent();

	//~ UActorComponent interface
	// Replicated property list: DropInitialVelocity (5.7 UHT only auto-declares
	// the function - the implementation must be provided, see the cpp).
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End of UActorComponent interface

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Starts (or restarts) the ballistic drop from the owner's current
	 *  location. Server side: rolls the random scatter, runs the arc and
	 *  replicates the launch (start point + initial velocity) to clients. */
	UFUNCTION(BlueprintCallable, Category = "Sn|Drop Motion")
	void LaunchDrop();

	/** Runs the arc with an explicit initial velocity - shared by the server
	 *  (after rolling the scatter) and clients (on receiving the replicated
	 *  velocity), so both sides simulate one deterministic trajectory. */
	void LaunchDropWithVelocity(const FVector& InVelocity);

	/** True once the owner has landed and switched to the float animation. */
	UFUNCTION(BlueprintPure, Category = "Sn|Drop Motion")
	bool IsSettled() const { return bSettled; }

	/** Upward impulse applied on launch (plus a random horizontal scatter). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float SpawnImpulse = 400.f;

	/** Random horizontal nudge range on launch (±cm) so drops scatter. */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float HorizontalScatter = 120.f;

	/** Gravity applied to the arc (cm/s^2). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float Gravity = 980.f;

	/** Vertical speed kept after the bounce (fraction). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float BounceRestitution = 0.4f;

	/** Fall speed under which the arc settles on ground contact instead of bouncing (-cm/s). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float BounceMinSpeed = -200.f;

	/** Fallback: force floating after this long if the owner never lands. */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float BallisticTimeout = 3.f;

	/** Rotation speed while floating (degrees per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float FloatRotateSpeed = 60.f;

	/** Bob frequency while floating (cycles per second). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float FloatBobSpeed = 2.f;

	/** Bob amplitude while floating (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float FloatBobHeight = 10.f;

	/** Ground contact lifts the owner this high (cm, roughly the actor's base). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float LandLiftHeight = 22.f;

	/** How far the ground probe reaches below the owner each frame (cm). */
	UPROPERTY(EditDefaultsOnly, Category = "Drop Motion")
	float TraceProbeDepth = 50.f;

private:
	/** Client side: the server's initial velocity arrived - replay the arc. */
	UFUNCTION()
	void OnRep_DropInitialVelocity();

	/** Ends the arc at the given settled location and starts the float animation. */
	void EnterFloatMode(const FVector& InSettledLocation);

	/** Timeout: settle in place if the owner never lands (e.g. dropped off a ledge). */
	void HandleBallisticTimeout();

	/** Where this drop started, as decided by the server - the other half of the
	 *  replicated launch. A pooled pickup is teleported to its new spot on the
	 *  server and that move never replicates, so a client copy would otherwise
	 *  run its arc from wherever it was left (the PREVIOUS drop's landing spot).
	 *  Same start + same velocity = the same arc on every machine. */
	UPROPERTY(Replicated)
	FVector DropStartLocation;

	/** The initial arc velocity as rolled by the server; replicated so every
	 *  client replays the same trajectory locally (see class comment). */
	UPROPERTY(ReplicatedUsing = OnRep_DropInitialVelocity)
	FVector DropInitialVelocity;

	/** True once the owner has been launched (velocity known - own or
	 *  replicated); the arc does not run before that. */
	bool bLaunched = false;

	/** Arc state (used until the owner lands). */
	FVector BallisticVelocity;
	int32 BounceCount = 0;
	bool bSettled = false;
	FVector SettledLocation;
	float FloatPhase = 0.f;

	FTimerHandle SettleTimerHandle;
};
