// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputMappingContext.h"
#include "SntpPlayerController.generated.h"

/**
 * 
 */
UCLASS()
class SNTP_API ASntpPlayerController : public APlayerController
{
	GENERATED_BODY()
	
public:
	ASntpPlayerController();
	virtual void SetupInputComponent() override;
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere, Category="Input")
	UInputAction* MoveAction;
	
	void HandleMove(const FInputActionValue& InputValue);
	
private:
	UPROPERTY(EditAnywhere, Category="Input")
	TSoftObjectPtr<UInputMappingContext> InputMapping;
	
	float Speed = 500.f;
	
};
