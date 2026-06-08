// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SntpPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Characters/SntpPlayerCharacter.h"
#include "Components/BuildingComponent/BuildableManagerComponent.h"
#include "Components/ComboComponent/ComboComponent.h"
#include "Input/SntpInputComponent.h"
#include "GameFramework/Character.h"
#include "Interaction/EnemyInterface.h"
#include "UI/SntpHUD.h"
#include "UI/Widget/DamageTextComponent.h"
#include "UI/WidgetController/OverlayWidgetController.h"


ASntpPlayerController::ASntpPlayerController()
{
	bReplicates = true;
	
	bAutoManageActiveCameraTarget = true;
	
}

void ASntpPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	// CursorTrace();
	
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
	 * Build System: Place and Switch Build mode
	 */
	SntpInputComponent->BindAction(SwitchBuildModeAction, ETriggerEvent::Started, this, &ASntpPlayerController::ToggleBuildMode);
	SntpInputComponent->BindAction(PlaceAction, ETriggerEvent::Started, this, &ASntpPlayerController::HandlePlace);
	
	/**
	 * Setting Menu
	 */
	SntpInputComponent->BindAction(SettingMenuAction, ETriggerEvent::Started, this, &ASntpPlayerController::HandleSettingMenu);
	
	/**
	 * Alt
	 */
	SntpInputComponent->BindAction(AltPressedAction, ETriggerEvent::Started, this, &ASntpPlayerController::EnterUIMode);
	// SntpInputComponent->BindAction(AltReleasedAction, ETriggerEvent::Completed, this, &ASntpPlayerController::ExitUIMode);
}

void ASntpPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
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
	if (bIgnoreMoveInput)
	{
		return;
	}
	
	const FVector2D MoveInput = InputValue.Get<FVector2D>();
	
	APawn* PawnCharacter = GetPawn();
	if (!PawnCharacter) return;
	
	if (ASntpPlayerCharacter* SntpPlayer = Cast<ASntpPlayerCharacter>(PawnCharacter))
	{
		if (UComboComponent* ComboComponent = SntpPlayer->GetComboComponent())
		{
			if (ComboComponent->GetComboWindowOpen() == true)
			{
				return;
			}
			ComboComponent->Interrupt();
		}
	}

	const FRotator ControlRot = GetControlRotation();
	const FRotator YawRot(0.f, ControlRot.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	
	FVector MovementDir = (Forward * MoveInput.Y + Right * MoveInput.X).GetSafeNormal();

	PawnCharacter->AddMovementInput(MovementDir);
}

void ASntpPlayerController::HandleLook(const FInputActionValue& InputValue)
{
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
	if (GetASC() == nullptr)
	{
		return;
	}
	GetASC()->AbilityInputTagHeld(InputTag);
	if (CanPreInput)
	{
		PreInputTag = InputTag;
	}
}

void ASntpPlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
	GEngine->AddOnScreenDebugMessage(2, 3.f, FColor::Blue, *InputTag.ToString());
}

void ASntpPlayerController::AbilityInputTagHeld(FGameplayTag InputTag)
{
	
	/*if (GetASC() == nullptr)
	{
		return;
	}
	GetASC()->AbilityInputTagHeld(InputTag);
	if (CanPreInput)
	{
		PreInputTag = InputTag;
	}*/
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

void ASntpPlayerController::ToggleBuildMode()
{
	GetPawn<ASntpPlayerCharacter>()->BuildManager->ToggleBuildMode();
}

void ASntpPlayerController::HandlePlace()
{
	GetPawn<ASntpPlayerCharacter>()->BuildManager->TryPlaceBuildable(GetPawn<ASntpPlayerCharacter>()->BuildManager->CurrentBuildPreviewIndex);
	
}

void ASntpPlayerController::EnterUIMode()
{
	/*bShowMouseCursor = true;
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	
	SetIgnoreLookInput(true);
	SetIgnoreMoveInput(true);*/
	
	if (bIsInUIMode == false)
	{
		ToggleUIMode(true);
	}
	else
	{
		ToggleUIMode(false);
	}
}

void ASntpPlayerController::ExitUIMode()
{
	/*bShowMouseCursor = false;
	FInputModeGameOnly InputMode;
	InputMode.SetConsumeCaptureMouseDown(true);
	SetInputMode(InputMode);
	SetIgnoreLookInput(false);
	SetIgnoreMoveInput(false);*/
	
	ToggleUIMode(false);

}

void ASntpPlayerController::ToggleUIMode(bool bEnable)
{
	// UE_LOG(LogTemp, Warning, TEXT("ToggleUIMode: if this refresh too often, check the player controller."))
	if (bEnable == bIsInUIMode) return;
	if (!bEnable)
	{
		bShowMouseCursor = false;
		FInputModeGameOnly InputMode;
		InputMode.SetConsumeCaptureMouseDown(true);
		
		SetInputMode(InputMode);
		SetIgnoreLookInput(false);
		SetIgnoreMoveInput(false);
		bIsInUIMode = false;
		
		bEnableClickEvents = true;
		bEnableMouseOverEvents = true;
	}
	else
	{
		bShowMouseCursor = true;
		FInputModeGameAndUI InputMode;
		
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		// InputMode.SetHideCursorDuringCapture(false);
		SetInputMode(InputMode);
		// if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
		{
			FVector2D ViewportSize;
			GEngine->GameViewport->GetViewportSize(ViewportSize);

			float CenterX = ViewportSize.X / 2.0f;
			float CenterY = ViewportSize.Y / 2.0f;

			SetMouseLocation(CenterX, CenterY);
		}
		bIsInUIMode = true;
		
		bEnableClickEvents = false;
		bEnableMouseOverEvents = false;
	}
}
