// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/SntpPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Interaction/EnemyInterface.h"


ASntpPlayerController::ASntpPlayerController()
{
	bReplicates = true;
}

void ASntpPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	CursorTrace();
}

void ASntpPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	if (UEnhancedInputComponent* Input = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (MoveAction)
		{
			Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ASntpPlayerController::HandleMove);
		}
	}
}

void ASntpPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	
	if (ULocalPlayer* LocalPlayer = Cast<ULocalPlayer>(Player))
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

void ASntpPlayerController::CursorTrace()
{
	FHitResult Hit;
	GetHitResultUnderCursor(ECollisionChannel::ECC_Visibility, false, Hit);
	if (!Hit.bBlockingHit)
	{
		return;
	}
	LastActor = CurrentActor;
	CurrentActor = Cast<IEnemyInterface>(Hit.GetActor());
	if (CurrentActor != LastActor)
	{
		if (LastActor != nullptr)
		{
			LastActor->UnHighlightActor();
		}
		if (CurrentActor != nullptr)
		{
			CurrentActor->HighlightActor();
		}
	}
}
