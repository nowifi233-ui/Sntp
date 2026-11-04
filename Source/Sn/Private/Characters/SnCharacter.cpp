// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/SnCharacter.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/SnAttributeSet.h"
#include "AbilitySystem/SnGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Game/SnSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interaction/UPlayerInteractionComponent.h"
#include "Inventory/USnInventoryComponent.h"
#include "Inventory/USnItemLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Player/SnPlayerController.h"
#include "Player/SnPlayerState.h"
#include "TimerManager.h"
#include "UI/Widget/SnChatBubbleWidget.h"

// Sets default values
ASnCharacter::ASnCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Free-camera third person: the mouse drives the camera only, the body turns
	// toward the direction it actually runs (RotationRate = the turn speed, the
	// standard Epic third-person pairing). Camera-locked yaw - the old setting -
	// spun the idle character with every mouse move, and with no turn-in-place
	// animation to cover it that read as sliding across the screen. Strikes read
	// the aim off the controller instead of turning the body, so they still land
	// where the player is looking - and only the basic attack turns the body onto
	// that aim, because its swing should read as hitting what the player aimed
	// at: see USnGA_DamageSkill::GetAvatarAimRotation /
	// USnGA_SphereAttack::ShouldFaceAimOnStrike.
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f);

	// Overlap other pawns instead of blocking them, matching the enemies:
	// a blocking capsule on either side stops the chase ~1m out and the
	// enemies can never reach their bite range.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// Third-person camera: the boom follows the controller rotation (yaw + pitch),
	// the camera is a child of the boom and inherits its transform.
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = NormalArmLength;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);

	InteractionComponent = CreateDefaultSubobject<UPlayerInteractionComponent>(TEXT("InteractionComponent"));
	CombatModeComponent = CreateDefaultSubobject<UCombatModeComponent>(TEXT("CombatModeComponent"));
	BuildPreviewComponent = CreateDefaultSubobject<USnBuildPreviewComponent>(TEXT("BuildPreviewComponent"));

	// Hotbar held-item visuals: a 3D mesh (the item row's WorldMesh) or, as
	// fallback, the item-pickup icon card (plane mesh + two-sided M_ItemCard,
	// both from Sn Settings). Both mount on the hand socket, but the socket
	// needs the blueprint's skeletal mesh (post-construction), so they attach
	// lazily in RefreshHeldItem and start hidden on the mesh root. Pure local
	// visuals - the selection they derive from is replicated, this
	// representation never is.
	HeldItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldItemMesh"));
	HeldItemMesh->SetupAttachment(GetMesh());
	HeldItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldItemMesh->SetVisibility(false);

	HeldItemCard = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeldItemCard"));
	HeldItemCard->SetupAttachment(GetMesh());
	HeldItemCard->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HeldItemCard->SetVisibility(false);

	// Chat bubble: the enemies' status widget setup (Screen space, so the text
	// keeps its size and never rolls with the body). The widget class arrives
	// in BeginPlay (ChatBubbleWidgetClass, same split as ASnEnemy); the
	// component is created here so every machine has it and only the message
	// has to travel. Nothing ticks on it by default - the bubble is switched on
	// and off by the character, and the blueprint can enable ticking if it
	// wants an animated pop.
	ChatBubbleComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChatBubbleComponent"));
	ChatBubbleComponent->SetupAttachment(GetMesh());
	ChatBubbleComponent->SetWidgetSpace(EWidgetSpace::Screen);
	ChatBubbleComponent->SetRelativeLocation(FVector(0.f, 0.f, 200.f));
	ChatBubbleComponent->SetDrawSize(FVector2D(360.f, 90.f));

	// Chase target marker: enemies (SnEnemyAIController) acquire targets by
	// scanning for this plain actor tag instead of trusting
	// GetPlayerController()->GetPawn(), which is null right after a level
	// travel (continue game) and used to leave every enemy standing still.
	Tags.Add(SnGameplayTags::Actor_Player);
}

UAbilitySystemComponent* ASnCharacter::GetAbilitySystemComponent() const
{
	const ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>();
	return SnPS ? SnPS->GetAbilitySystemComponent() : nullptr;
}

USnAttributeSet* ASnCharacter::GetAttributeSet() const
{
	const ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>();
	return SnPS ? SnPS->GetAttributeSet() : nullptr;
}

void ASnCharacter::OnPlayerStateChanged(APlayerState* NewPlayerState, APlayerState* OldPlayerState)
{
	Super::OnPlayerStateChanged(NewPlayerState, OldPlayerState);

	// The engine calls this on BOTH sides whenever SetPlayerState runs on this
	// pawn - Pawn.h's own words: "Called on both the client and server when ever
	// SetPlayerState is called on this pawn" (Pawn.cpp:668). The server gets
	// here from PossessedBy, which copies Controller->PlayerState in
	// (Pawn.cpp:685, and that state is spawned back in
	// APlayerController::PostInitializeComponents - PlayerController.cpp:1076 -
	// long before RestartPlayer); the client gets here from OnRep_PlayerState
	// (Pawn.cpp:647). One override therefore covers both possess paths, and it
	// is the honest map of how possession works: the client's possession is
	// ClientRestart, which does SetPawn + SetController and calls Possess
	// nowhere at all (PlayerController.cpp:816-860 in 5.8; APawn::SetController
	// is a plain assignment, Pawn.cpp:1318) - so a client-side PossessedBy hook
	// would simply never run.
	//
	// UnPossessed clears the state and re-enters with a null NewPlayerState;
	// InitializeAbilitySystem's guard makes that a no-op.
	InitializeAbilitySystem();
}

void ASnCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	// The controller can land after the player state - the two are separate
	// replicated properties of this pawn, so either order is possible, and the
	// client also sets it locally from ClientRestart, which calls this very rep
	// notify (PlayerController.cpp:846-856). The ASC link above does not need
	// it, but the UI notification does: retry here, or a client whose player
	// state arrived first would initialize the ASC and then never tell the HUD.
	// Idempotent, and no-ops once the controller is set (this also fires with a
	// null controller on unpossess - the notify cast then simply fails).
	InitializeAbilitySystem();
}

void ASnCharacter::InitializeAbilitySystem()
{
	if (!bAbilitySystemInitialized)
	{
		ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>();
		if (!SnPS)
		{
			// The state has not replicated yet; OnPlayerStateChanged retries.
			return;
		}

		SnPS->InitAbilityActorInfo(this);
		// Only mark initialized once the ASC link actually happened - a client
		// that possessed before the player state replicated re-runs here from
		// OnPlayerStateChanged.
		bAbilitySystemInitialized = true;
	}

	// Aura pattern: the UI overlay must not initialize before the player state /
	// ASC / attribute set exist - HUD BeginPlay can beat the client's PlayerState
	// replication and would cache nulls forever. The notification therefore
	// fires on every readiness event rather than only on the first: it needs the
	// controller too, and the controller is a separate replication that can
	// arrive later (OnRep_Controller). The gate and InitializeOverlay are both
	// idempotent, so the repeats cost nothing.
	if (ASnPlayerController* SnPC = Cast<ASnPlayerController>(GetController()))
	{
		SnPC->TryInitializeUI();
	}

	// Hotbar held-item visuals: re-derive from the inventory's hotbar state.
	// Bound here, not in BeginPlay - the player state is guaranteed present by
	// this point, and the binding is idempotent (RemoveAll first), so re-running
	// it on a later readiness event or a re-possess never double-binds. The
	// refresh covers selections made before the bind (replicated state can
	// arrive ahead of this pawn).
	if (ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>())
	{
		if (USnInventoryComponent* Inventory = SnPS->FindComponentByClass<USnInventoryComponent>())
		{
			Inventory->OnHotbarChanged.RemoveAll(this);
			Inventory->OnHotbarChanged.AddDynamic(this, &ASnCharacter::RefreshHeldItem);
			Inventory->OnInventoryChanged.RemoveAll(this);
			Inventory->OnInventoryChanged.AddDynamic(this, &ASnCharacter::RefreshHeldItem);
			RefreshHeldItem();
		}
	}

	BindAttributeCallbacks();
}

void ASnCharacter::BindAttributeCallbacks()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	// Idempotent: possessed/OnRep can re-run (server + client each run once,
	// but re-possession exists too). Unbind first so the flash fires exactly
	// once per health change. Same pattern as ASnEnemy.
	ASC->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetHealthAttribute())
		.RemoveAll(this);
	ASC->GetGameplayAttributeValueChangeDelegate(USnAttributeSet::GetHealthAttribute())
		.AddUObject(this, &ASnCharacter::HandleHealthChanged);
}

void ASnCharacter::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	// Only flash on damage - healing (potion, level-up) must not flash red.
	if (Data.NewValue < Data.OldValue)
	{
		FlashRed();
	}
}

void ASnCharacter::FlashRed()
{
	if (HitFlashMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(HitFlashMaterial);
		GetWorldTimerManager().SetTimer(FlashTimerHandle, this, &ASnCharacter::ClearFlash, FlashDuration, false);
	}
}

void ASnCharacter::ClearFlash()
{
	if (GetMesh())
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}
}

void ASnCharacter::MulticastShowChatBubble_Implementation(const FString& Message)
{
	// Runs on the server and on every client the pawn is relevant to. Each end
	// shows its own bubble - no widget, no timer and no "currently shown" state
	// crosses the wire, so a late joiner cannot be told a stale message and a
	// bubble can never double-fire on a machine.
	ShowChatBubble(Message);
}

void ASnCharacter::ShowChatBubble(const FString& Message)
{
	// No widget (no class configured, or nothing to draw on this machine -
	// dedicated server): the line is simply not shown. Never an error.
	if (!ChatBubbleWidget)
	{
		return;
	}

	// FromString keeps the text plain: another player's line must never be
	// parsed as markup or run through the localization pipeline.
	ChatBubbleWidget->ShowMessage(FText::FromString(Message));

	// Restart, not extend: a second message replaces the first and gets the
	// full duration. SetTimer resets an active timer, so a fast talker keeps
	// one bubble up rather than stacking timers that would cut it short.
	GetWorldTimerManager().SetTimer(ChatBubbleTimerHandle, this, &ASnCharacter::HideChatBubble, ChatBubbleDuration, false);
}

void ASnCharacter::HideChatBubble()
{
	if (ChatBubbleWidget)
	{
		ChatBubbleWidget->HideMessage();
	}
}

void ASnCharacter::RefreshHeldItem()
{
	// Derive the requested display from the replicated hotbar selection:
	// only while the bag still holds the item (reference semantics - an
	// emptied stack or an unknown row hides the hand).
	FName RowName = NAME_None;
	const FSnItemRow* Row = nullptr;
	if (USnInventoryComponent* Inventory = GetInventoryComponent())
	{
		RowName = Inventory->GetSelectedHotbarRowName();
		if (!RowName.IsNone() && Inventory->GetItemCount(RowName) > 0)
		{
			Row = USnItemLibrary::FindItemRow(RowName);
		}
	}
	const EHeldDisplay Want = !Row ? EHeldDisplay::None
		: (Row->WorldMesh.IsNull() ? EHeldDisplay::Card : EHeldDisplay::Mesh);

	// Diff: identical state leaves the current attach untouched. Both source
	// delegates fire often (every inventory replication), and the queries
	// above are cheap, so this keeps the swap to actual changes only.
	if (Want == HeldDisplay && ShownHeldRowName == RowName)
	{
		return;
	}

	HeldDisplay = Want;
	ShownHeldRowName = Row ? RowName : NAME_None;

	if (!HeldItemMesh || !HeldItemCard)
	{
		return;
	}

	if (Want == EHeldDisplay::None)
	{
		HeldItemMesh->SetVisibility(false);
		HeldItemCard->SetVisibility(false);
		return;
	}

	if (Want == EHeldDisplay::Mesh)
	{
		HeldItemCard->SetVisibility(false);
		if (UStaticMesh* Mesh3D = Row->WorldMesh.LoadSynchronous())
		{
			HeldItemMesh->SetStaticMesh(Mesh3D);
			AttachHeldToSocket(HeldItemMesh);
			HeldItemMesh->SetVisibility(true);
		}
		return;
	}

	// Icon-card fallback - the same look as the item's ground drop: the pickup
	// plane mesh + two-sided M_ItemCard with the icon in its "CardTexture"
	// parameter. One MID per character, only the texture changes per item
	// switch. Nothing shows while the mesh/material aren't configured yet
	// (Sn Settings assignment is an editor step).
	HeldItemMesh->SetVisibility(false);
	HeldItemCard->SetVisibility(false);
	if (UTexture2D* Icon = USnItemLibrary::GetItemIconLoaded(RowName))
	{
		if (!HeldItemCard->GetStaticMesh())
		{
			if (const USnSettings* Settings = USnSettings::Get())
			{
				HeldItemCard->SetStaticMesh(Settings->GetIconCardMesh());
			}
		}
		if (!HeldCardMID)
		{
			if (const USnSettings* Settings = USnSettings::Get())
			{
				if (UMaterialInterface* Material = Settings->GetIconCardMaterial())
				{
					HeldCardMID = UMaterialInstanceDynamic::Create(Material, this);
					HeldItemCard->SetMaterial(0, HeldCardMID);
				}
			}
		}
		if (HeldItemCard->GetStaticMesh() && HeldCardMID)
		{
			HeldCardMID->SetTextureParameterValue(TEXT("CardTexture"), Icon);
			AttachHeldToSocket(HeldItemCard);
			// SnapToTarget resets the relative scale on attach - the card mesh
			// (the pickup plane) is far bigger than hand size, so the character
			// scale lands after every attach.
			HeldItemCard->SetRelativeScale3D(HeldCardScale);
			HeldItemCard->SetVisibility(true);
		}
	}
}

USnInventoryComponent* ASnCharacter::GetInventoryComponent() const
{
	const ASnPlayerState* SnPS = GetPlayerState<ASnPlayerState>();
	return SnPS ? SnPS->FindComponentByClass<USnInventoryComponent>() : nullptr;
}

void ASnCharacter::AttachHeldToSocket(USceneComponent* Component) const
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!Component || !SkeletalMesh)
	{
		return;
	}

	// SnapToTarget: the held item appears exactly at the socket each time the
	// display switches to it. A mesh without the socket degrades to its root
	// (visible at the hand-ish origin) instead of logging attach failures.
	const FName Socket = SkeletalMesh->DoesSocketExist(HandSocketName) ? HandSocketName : NAME_None;
	Component->AttachToComponent(SkeletalMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, Socket);
}

// Called when the game starts or when spawned
void ASnCharacter::BeginPlay()
{
	Super::BeginPlay();

	// Overhead chat bubble: the widget class comes from the BP defaults, so it
	// is applied at runtime - the same shape ASnEnemy uses for its status
	// widget (one actor-level field instead of a component property to forget).
	// Runs on every machine with Slate; the widget itself never replicates.
	if (ChatBubbleComponent && ChatBubbleWidgetClass)
	{
		ChatBubbleComponent->SetWidgetClass(ChatBubbleWidgetClass);
		ChatBubbleComponent->InitWidget();
		ChatBubbleWidget = Cast<USnChatBubbleWidget>(ChatBubbleComponent->GetUserWidgetObject());

		if (ChatBubbleWidget)
		{
			// Hidden until the first line arrives: the starting state is C++'s
			// call, so a blueprint whose root is Visible cannot leave an empty
			// bubble frame floating over every character from spawn.
			ChatBubbleWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

// Called every frame
void ASnCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Combat camera: smoothly zoom out while in combat, back out when not
	// (reads the local combat state, so multiplayer never fights over it).
	if (CameraBoom)
	{
		const float TargetLength = CombatModeComponent && CombatModeComponent->IsInCombat() ? CombatArmLength : NormalArmLength;
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, TargetLength, DeltaTime, ArmLengthInterpSpeed);
	}
}

// Called to bind functionality to input
void ASnCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

