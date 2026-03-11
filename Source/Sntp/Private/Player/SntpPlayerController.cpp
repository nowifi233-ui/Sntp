// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SntpPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "EnhancedInputSubsystems.h"
#include "Input/SntpInputComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "GameFramework/Character.h"
#include "Input/SntpInputComponent.h"
#include "Interaction/EnemyInterface.h"
#include "UI/Widget/DamageTextComponent.h"


ASntpPlayerController::ASntpPlayerController()
{
	bReplicates = true;
}

void ASntpPlayerController::PlayerTick(const float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
}

void ASntpPlayerController::ShowDamageNumber_Implementation(const float DamageAmount, ACharacter* TargetCharacter)
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
		DamageTextComponent->SetDamageText(DamageAmount);
	}
}

void ASntpPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// Bind functions and Input Actions

	USntpInputComponent* SntpInputComponent = Cast<USntpInputComponent>(InputComponent);
	SntpInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASntpPlayerController::HandleMove);
	SntpInputComponent->BindAbilityActions(
		InputConfig,
		this,
		&ThisClass::AbilityInputTagPressed,
		&ThisClass::AbilityInputTagReleased,
		&ThisClass::AbilityInputTagHeld);
	
}

void ASntpPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Set Mouse actions
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	
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
	// Moving action for bind.
	const FVector2D MoveInput = InputValue.Get<FVector2D>();
    const FRotator Rotation = GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);
	
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	const FVector RightDirection = FRotationMatrix(Rotation).GetUnitAxis(EAxis::X);
	
	if (APawn* ControlledPawn = GetPawn<APawn>())
	{
		ControlledPawn->AddMovementInput(ForwardDirection, MoveInput.X);
		ControlledPawn->AddMovementInput(RightDirection, MoveInput.Y);
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
