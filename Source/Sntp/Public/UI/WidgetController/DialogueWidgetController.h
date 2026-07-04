// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/DialogueComponent/DialogueComponent.h"
#include "Misc/NotNull.h"
#include "UI/WidgetController/SntpWidgetController.h"
#include "DialogueWidgetController.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FChoiceList
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<FDialogueChoice> Choices;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FDialogueWidgetDelegate,
	FText, Name,
	FText, Text,
	FChoiceList, ChoiceList
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
	void DialogueOptionChosen(FDialogueChoice Choice);
	
	UFUNCTION(BlueprintCallable)
	void StartDialogue();
	
	UFUNCTION(BlueprintCallable)
	void StopDialogue();
	
	UFUNCTION()
	void OnDialogueChanged(FText Name, FText Text, TArray<FDialogueChoice> Choices);
	
	UPROPERTY(BlueprintAssignable)
	FDialogueWidgetDelegate OnDialogueChangedDelegate;
};
