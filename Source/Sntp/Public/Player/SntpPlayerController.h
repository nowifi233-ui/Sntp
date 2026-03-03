// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "SntpPlayerController.generated.h"

class IEnemyInterface;
/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ASntpPlayerController();
	virtual void PlayerTick(const float DeltaTime) override;

protected:
	virtual void BeginPlay() override;
	
	// Bind input actions and functions 
	virtual void SetupInputComponent() override;
	
private:
	// Enhanced Input Subsystem
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;
	
	void HandleMove(const FInputActionValue& InputValue);

	UPROPERTY(EditAnywhere, Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
	
	float Speed = 500.f;
	
	// Highlight enemies
	void CursorTrace();
	IEnemyInterface* CurrentActor;
	IEnemyInterface* LastActor;
};
