// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SntpPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Characters/SntpPlayerCharacter.h"
#include "Input/SntpInputComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GridSystem/PreviewActor.h"
#include "Interaction/EnemyInterface.h"
#include "UI/SntpHUD.h"
#include "UI/Widget/DamageTextComponent.h"
#include "UI/WidgetController/OverlayWidgetController.h"


ASntpPlayerController::ASntpPlayerController()
{
	bReplicates = true;
}

void ASntpPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
	
	/**
	 * Grid Building System Preview
	 */
	if (BuildState == EBuildModeState::Placing)
	{
		UpdatePreview();
	}
}

void ASntpPlayerController::ShowDamageNumber_Implementation(const float DamageAmount, ACharacter* TargetCharacter, bool bCritical)
{
	if (IsValid(TargetCharacter) && DamageTextComponentClass && IsLocalController())
	{
		UDamageTextComponent* DamageTextComponent = NewObject<UDamageTextComponent>(TargetCharacter, DamageTextComponentClass);
		DamageTextComponent->RegisterComponent();
		DamageTextComponent->AttachToComponent(TargetCharacter->GetRootComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale);
		const FVector DamageWorldLocation = DamageTextComponent->GetComponentLocation();
		const FRotator DamageWorldRotation = DamageTextComponent->GetComponentRotation();
		
		DamageTextComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
		DamageTextComponent->SetWorldLocationAndRotation(DamageWorldLocation, DamageWorldRotation);
		DamageTextComponent->SetDamageText(DamageAmount, bCritical);
	}
}

void ASntpPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Bind functions and Input Actions

	USntpInputComponent* SntpInputComponent = Cast<USntpInputComponent>(InputComponent);
	SntpInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASntpPlayerController::HandleMove);
	SntpInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ASntpPlayerController::HandleLook);
	
	/**
	 * Interaction System
	 */
	SntpInputComponent->BindAction(ScrollAction, ETriggerEvent::Triggered, this, &ASntpPlayerController::HandleScroll);
	SntpInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &ASntpPlayerController::HandleInteract);
	SntpInputComponent->BindAction(ToggleInventoryAction, ETriggerEvent::Started, this, &ASntpPlayerController::ToggleInventory);
	/**
	 * Ability System
	 */
	SntpInputComponent->BindAbilityActions(
		InputConfig,
		this,
		&ThisClass::AbilityInputTagPressed,
		&ThisClass::AbilityInputTagReleased,
		&ThisClass::AbilityInputTagHeld);
	
	/**
	 * Grid Build System: Place and Switch Build mode
	 */
	SntpInputComponent->BindAction(SwitchBuildModeAction, ETriggerEvent::Started, this, &ASntpPlayerController::ToggleBuildMode);
	SntpInputComponent->BindAction(PlaceAction, ETriggerEvent::Started, this, &ASntpPlayerController::HandlePlace);
	
	/**
	 * Setting Menu
	 */
	SntpInputComponent->BindAction(SettingMenuAction, ETriggerEvent::Started, this, &ASntpPlayerController::HandleSettingMenu);
}

void ASntpPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Spawn Preview Actor
	PreviewActor = GetWorld()->SpawnActor<APreviewActor>(PreviewClass);
	GridManager = AGridManager::Get(GetWorld());
	
	// Set Mouse actions
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true);
	SetInputMode(InputMode);
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);
	
	// Set up Input Mapping context
	if (const ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (!InputMapping.IsNull())
			{
				InputSystem->AddMappingContext(InputMapping.LoadSynchronous(), 1);
			}
		}
	}
}



void ASntpPlayerController::HandleMove(const FInputActionValue& InputValue)
{
	const FVector2D MoveInput = InputValue.Get<FVector2D>();

	APawn* PawnCharacter = GetPawn();
	if (!PawnCharacter) return;

	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	
	FVector MovementDir = (Forward * MoveInput.Y + Right * MoveInput.X).GetSafeNormal();

	PawnCharacter->AddMovementInput(MovementDir);
}

void ASntpPlayerController::HandleLook(const FInputActionValue& InputValue)
{
	if (BuildState == EBuildModeState::Placing)
	{
		return;
	}
	const FVector2D LookInput = InputValue.Get<FVector2D>();
	AddYawInput(LookInput.X);
	AddPitchInput(LookInput.Y);
}

void ASntpPlayerController::HandleScroll(const FInputActionValue& InputValue)
{
	const float ScrollInput = InputValue.Get<float>();
	if (UOverlayWidgetController* OverlayWidgetController = Cast<ASntpHUD>(GetHUD())->GetOverlayWidgetControllerWithoutParam())
	{
		int32 IntScroll = ScrollInput;
		OverlayWidgetController->OnScrollDelegate.Broadcast(IntScroll);
	}
}

void ASntpPlayerController::HandleInteract(const FInputActionValue& InputValue)
{
	const float ScrollInput = InputValue.Get<bool>();
	if (ScrollInput)
	{
		if (UOverlayWidgetController* OverlayWidgetController = Cast<ASntpHUD>(GetHUD())->GetOverlayWidgetControllerWithoutParam())
		{
			FInteractionOption Option = OverlayWidgetController->CurrentOption;
			if (const ASntpPlayerCharacter* PlayerCharacter = GetPawn<ASntpPlayerCharacter>())
			{
				PlayerCharacter->InteractionComponent->Interact(Option);
				OverlayWidgetController->OnOptionSelected.Broadcast(Option);
			}
		}
	}
}

void ASntpPlayerController::ToggleInventory(const FInputActionValue& InputValue)
{
	ASntpHUD* HUD = GetHUD<ASntpHUD>();
	if (HUD)
	{
		HUD->ToggleBag(this);
	}
}

void ASntpPlayerController::HandleSettingMenu(const FInputActionValue& InputValue)
{
	ASntpHUD* HUD = GetHUD<ASntpHUD>();
	if (HUD)
	{
		HUD->ToggleSettingMenu(this);
	}
}

void ASntpPlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	GEngine->AddOnScreenDebugMessage(1, 3.f, FColor::Red, *InputTag.ToString());
}

void ASntpPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Blue, *InputTag.ToString());
}

void ASntpPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	if (GetASC() == nullptr)
	{
		return;
	}
	GetASC()->AbilityInputTagHeld(InputTag);
}

USntpAbilitySystemComponent* ASntpPlayerController::GetASC()
{
	if (SntpAbilitySystemComponent == nullptr)
	{
		SntpAbilitySystemComponent = Cast<USntpAbilitySystemComponent>(UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn<APawn>()));
	}
	return SntpAbilitySystemComponent;
	
}

void ASntpPlayerController::CursorTrace()
{
	// Get Current and Last actor under cursor. 
	FHitResult Hit;
	GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, Hit);
	if (!Hit.bBlockingHit)
	{
		return;
	}
	LastActor = CurrentActor;
	CurrentActor = Cast<IEnemyInterface>(Hit.GetActor());
	// Highlight and unhighlight actors refer location of cursor.
	if (CurrentActor != LastActor)
	{
		if (LastActor != nullptr) LastActor->UnHighlightActor();
		if (CurrentActor != nullptr) CurrentActor->HighlightActor();
	}
}

void ASntpPlayerController::UpdatePreview()
{
	if (!PreviewActor || !GridManager)
	{
		return;
	}
	
	ABuildingBase* BuildCDO = CurrentBuildClass->GetDefaultObject<ABuildingBase>();
	if (BuildCDO && BuildCDO->Mesh)
	{
		PreviewActor->Mesh->SetStaticMesh(BuildCDO->Mesh);
	}
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	if (!Hit.bBlockingHit) return;

	// ① 世界 → Grid
	FIntPoint Coord = GridManager->WorldToGrid(Hit.Location);

	// ② Grid → 世界（对齐格子）
	FVector SnapLocation = GridManager->GridToWorld(Coord);

	// ③ 移动预览Actor
	PreviewActor->SetActorLocation(SnapLocation);

	// ④ 判断能不能放
	bool bCanPlace = GridManager->CanPlace(Coord);

	// ⑤ 更新颜色
	PreviewActor->SetValid(bCanPlace);
}

void ASntpPlayerController::EnterBuildMode(TSubclassOf<AActor> BuildClass)
{
	BuildState = EBuildModeState::Placing;
	// CurrentBuildClass = BuildClass;
	
	// Preview Actor
	if (!PreviewActor)
	{
		PreviewActor = GetWorld()->SpawnActor<APreviewActor>(PreviewClass);
	}
	PreviewActor->SetActorHiddenInGame(false);
	GridManager->SetGridMeshVisible(true);
	
	
	// Switch Camrea
	ASntpPlayerCharacter* PlayerCharacter = GetPawn<ASntpPlayerCharacter>();
	PlayerCharacter->FollowCamera->Deactivate();
	PlayerCharacter->BuildCamera->Activate();
	// PlayerCharacter->GetCharacterMovement()->DisableMovement();
	
	// Set Mouse actions
	bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	
}

void ASntpPlayerController::ExitBuildMode()
{
	BuildState = EBuildModeState::None;
	// CurrentBuildClass = nullptr;
	if (PreviewActor)
	{
		PreviewActor->SetActorHiddenInGame(true);
	}
	GridManager->SetGridMeshVisible(false);
	
	ASntpPlayerCharacter* PlayerCharacter = GetPawn<ASntpPlayerCharacter>();
	PlayerCharacter->BuildCamera->Deactivate();
	PlayerCharacter->FollowCamera->Activate();
	// PlayerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	
	// Set Mouse actions
	bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true);
	SetInputMode(InputMode);
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);
}

void ASntpPlayerController::ToggleBuildMode()
{
	
	if (BuildState == EBuildModeState::None)
	{
		EnterBuildMode(PreviewClass);
	}
	else
	{
		ExitBuildMode();
	}
}

void ASntpPlayerController::HandlePlace()
{
	if (BuildState != EBuildModeState::Placing) return;
	FHitResult Hit;
	GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	
	if (!Hit.bBlockingHit) return;
	
	FIntPoint Coord = GridManager->WorldToGrid(Hit.Location);
	
	
	if (GridManager->CanPlace(Coord))
	{
		FActorSpawnParameters Params;
		GridManager->Place(Coord, CurrentBuildClass);
	}
	else
	{
		GEngine->AddOnScreenDebugMessage(2, 3, FColor::Red, *Hit.Location.ToString());
	}
}
