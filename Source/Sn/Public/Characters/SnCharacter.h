// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "Combat/UCombatModeComponent.h"
#include "Game/USnBuildPreviewComponent.h"
#include "GameFramework/Character.h"
#include "SnCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UAbilitySystemComponent;
class USnAttributeSet;
class UPlayerInteractionComponent;
class UMaterialInterface;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;
class USnInventoryComponent;
class USnChatBubbleWidget;
class UWidgetComponent;
struct FOnAttributeChangeData;
class APlayerState;

UCLASS()
class SN_API ASnCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	// Sets default values for this character's properties
	ASnCharacter();

	//~ IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~ End of IAbilitySystemInterface

	/** Returns the primary attribute set owned by the player state. */
	UFUNCTION(BlueprintPure, Category = "AbilitySystem")
	USnAttributeSet* GetAttributeSet() const;

	virtual void OnRep_Controller() override;

	/** Runs InitAbilityActorInfo + BindAttributeCallbacks once, from
	 *  OnPlayerStateChanged (the engine calls it on both sides) and
	 *  OnRep_Controller (whichever of the state and the controller lands last),
	 *  and re-runs the HUD's readiness gate every time - that one needs the
	 *  controller too, and it is a separate replication. Fully idempotent. */
	void InitializeAbilitySystem();

	/** Binds the health attribute change delegate (idempotent; the ASC lives on
	 *  the player state - call after InitAbilityActorInfo). */
	void BindAttributeCallbacks();

	/** Health went down: flash the mesh red (same as the enemies). */
	void HandleHealthChanged(const FOnAttributeChangeData& Data);

	/** Puts the hit-flash overlay on and schedules ClearFlash. */
	void FlashRed();

	/** Removes the hit-flash overlay. */
	void ClearFlash();

	/** Re-derives the held-item visual from the inventory's hotbar selection
	 *  (binds to OnHotbarChanged / OnInventoryChanged - dynamic delegate, so
	 *  UFUNCTION). Three-way switch per the selected row: 3D WorldMesh, icon
	 *  card fallback, or hidden when nothing is selected / the bag no longer
	 *  holds the item. Skips (no re-attach) while the state is unchanged. */
	UFUNCTION()
	void RefreshHeldItem();

	/** Relays one chat line to every machine: the server runs this on the
	 *  SPEAKING pawn, so the actor it executes on IS the speaker - nobody has
	 *  to resolve a player state on the receiving side to know whose head the
	 *  bubble belongs on. Called by ASnPlayerController::ServerSendChatMessage
	 *  after validation; never called from a client.
	 *
	 *  The bubble itself is NOT replicated (the widget component exists on
	 *  every machine with the class - components always do); the message is the
	 *  only thing that travels. That leaves exactly one writer per machine, so
	 *  there is no local UI state that can race a replication event. */
	UFUNCTION(NetMulticast, Reliable, Category = "Chat")
	void MulticastShowChatBubble(const FString& Message);

	/** Shows the bubble on this machine and (re)arms the hide timer. Every end
	 *  runs its own - the network carries the words, never the widget. */
	void ShowChatBubble(const FString& Message);

	/** Hides the bubble (bubble timer). */
	void HideChatBubble();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** The single entry point of the ability system link. The engine calls this
	 *  on BOTH sides whenever SetPlayerState runs on this pawn (Pawn.h) - the
	 *  server reaches it through PossessedBy, the client through
	 *  OnRep_PlayerState - so overriding it here covers both possess paths, and
	 *  there is no client-side possession to hook anyway (the client's
	 *  possession arrives as ClientRestart, which never calls Possess). */
	virtual void OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState) override;

	/** Camera boom positioning the camera behind the character. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** Third-person follow camera attached to the boom. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	/** Scans for nearby interactables and broadcasts them (timer-driven). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaction")
	TObjectPtr<UPlayerInteractionComponent> InteractionComponent;

	/** Local combat state: drives the camera zoom and the enemy health bars. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	TObjectPtr<UCombatModeComponent> CombatModeComponent;

	/** Ghost of the device the selected hotbar item would place (local only,
	 *  like the combat state above - the placement itself is server-side). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Building")
	TObjectPtr<USnBuildPreviewComponent> BuildPreviewComponent;

	/** Camera arm length while in combat (zoomed out, Genshin-style). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float CombatArmLength = 550.f;

	/** Camera arm length outside combat. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float NormalArmLength = 400.f;

	/** How fast the camera arm interpolates between the two lengths. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float ArmLengthInterpSpeed = 5.f;

	/** Overlay material flashed on the mesh when taking damage (same pattern
	 *  as ASnEnemy::FlashRed). */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	TObjectPtr<UMaterialInterface> HitFlashMaterial;

	/** How long the hit flash stays on. */
	UPROPERTY(EditDefaultsOnly, Category = "Combat")
	float FlashDuration = 0.12f;

	// --- Hotbar held item (visual only; the selection state lives on the
	// inventory component - it is replicated, this local derivation is not) ---

	/** Socket on the skeletal mesh the held item attaches to. */
	UPROPERTY(EditDefaultsOnly, Category = "Held Item")
	FName HandSocketName = TEXT("hand_r");

	/** 3D mesh shown while the selected hotbar item has a WorldMesh. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
	TObjectPtr<UStaticMeshComponent> HeldItemMesh;

	/** Icon-card fallback while the selected item has no WorldMesh: the same
	 *  plane + two-sided icon material (M_ItemCard) the item pickups use, so a
	 *  held item reads exactly like its ground drop. Mesh/material come from
	 *  Sn Settings (GetIconCardMesh / GetIconCardMaterial). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Held Item")
	TObjectPtr<UStaticMeshComponent> HeldItemCard;

	/** Dynamic instance of the icon-card material ("CardTexture" texture
	 *  parameter), kept across item switches - only the texture changes. */
	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> HeldCardMID;

	/** Scale applied to the icon card after every socket attach
	 *  (SnapToTargetIncludingScale resets the relative scale). The card mesh
	 *  from Sn Settings is the pickup plane - far bigger than hand size; tune
	 *  this until the held card reads like the ground drop. */
	UPROPERTY(EditDefaultsOnly, Category = "Held Item")
	FVector HeldCardScale = FVector(0.25f, 0.25f, 0.25f);

	// --- Chat bubble (overhead line, local presentation of a replicated
	// message; see MulticastShowChatBubble) ---

	/** Overhead chat bubble. Screen space (constant text size, never rolls with
	 *  the body) and pure local presentation: the widget class comes from
	 *  ChatBubbleWidgetClass below, the component exists on every machine, and
	 *  only the message travels - the widget is never replicated. Same setup as
	 *  the enemies' status widget. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Chat")
	TObjectPtr<UWidgetComponent> ChatBubbleComponent;

	/** Widget class for the overhead chat bubble (e.g. WBP_ChatBubble). Applied
	 *  to the component in BeginPlay - the same split ASnEnemy uses for its
	 *  status widget, so the class lives in one actor-level field instead of on
	 *  a component instance. Unset = chat lines are silently not shown. */
	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	TSubclassOf<USnChatBubbleWidget> ChatBubbleWidgetClass;

	/** How long a chat line stays up. */
	UPROPERTY(EditDefaultsOnly, Category = "Chat", meta = (ClampMin = "0.1"))
	float ChatBubbleDuration = 4.f;

private:
	/** True once InitAbilityActorInfo ran on this pawn (guards re-entry from
	 *  OnPlayerStateChanged and OnRep_Controller). */
	bool bAbilitySystemInitialized = false;

	FTimerHandle FlashTimerHandle;

	/** Hide timer of the chat bubble, on this character (not on the widget):
	 *  a second message must restart it, and the pawn outlives any message. */
	FTimerHandle ChatBubbleTimerHandle;

	/** The live bubble widget (built from ChatBubbleWidgetClass in BeginPlay).
	 *  Owned by the component - this is the cached handle, not a second owner. */
	UPROPERTY()
	TObjectPtr<USnChatBubbleWidget> ChatBubbleWidget;

	/** Which held display is currently active (diff key, see RefreshHeldItem). */
	enum class EHeldDisplay : uint8 { None, Mesh, Card };

	/** Row name currently shown on the hand; NAME_None while nothing is held. */
	FName ShownHeldRowName;

	EHeldDisplay HeldDisplay = EHeldDisplay::None;

	/** The player state's inventory component (hotbar selection + item count
	 *  source). nullptr until the player state exists. */
	USnInventoryComponent* GetInventoryComponent() const;

	/** Snaps a held component onto the hand socket. Missing socket (a mesh
	 *  without hand_r) degrades to the mesh root instead of failing. */
	void AttachHeldToSocket(USceneComponent* Component) const;

public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
