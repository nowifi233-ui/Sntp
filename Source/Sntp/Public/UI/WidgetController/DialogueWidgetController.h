// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/DialogueComponent/DialogueComponent.h"
#include "Misc/NotNull.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "DialogueWidgetController.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
	FDialogueWidgetDelegate,
	FText, Name,
	FText, Text
	);
/**
 * 
 */
UCLASS(Blueprintable, BlueprintType)
class SNTP_API UDialogueWidgetController : public USntpWidgetController
{
	GENERATED_BODY()
	
public:
	// In dialogue component;
	UPROPERTY()
	TObjectPtr<UDialogueComponent> DialogueComponent;
	
	UFUNCTION()
	void Init(UDialogueComponent* InDialogueComponent, APlayerController* InPlayerController);
	
	UFUNCTION()
	void SetDialogueComponent(UDialogueComponent* InDialogueComponent);
	
	UFUNCTION(BlueprintCallable)
	void NextDialogue();
	
	UFUNCTION(BlueprintCallable)
	void StartDialogue();
	
	UFUNCTION(BlueprintCallable)
	void StopDialogue();
	
	UFUNCTION()
	void OnDialogueChanged(FText Name, FText Text);
	
	UPROPERTY(BlueprintAssignable)
	FDialogueWidgetDelegate OnDialogueChangedDelegate;
};
