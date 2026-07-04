// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/FishingComponent/DialogueDataAsset.h"
#include "DialogueComponent.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStopDialogueDelegate);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
	FDialogueDelegate,
	FText, Name,
	FText, Text,
	TArray<FDialogueChoice>, Choices
	);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UDialogueComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UDialogueComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UPROPERTY()
	int CurrentIndex;
	
	UPROPERTY()
	FName CurrentID;
	
	UPROPERTY()
	UDialogueDataAsset* CurrentDialogue;
	
	UPROPERTY(BlueprintAssignable)
	FStopDialogueDelegate StopDialogue;
	
	UPROPERTY(BlueprintAssignable)
	FDialogueDelegate DialogueDelegate;
	
public:
	UFUNCTION(BlueprintCallable)
	void StartDialogue(UDialogueDataAsset* Data);
	
	UFUNCTION(BlueprintCallable)
	void ShowCurrentLine();
	
	UFUNCTION(BlueprintCallable)
	FText GetCurrentLine();
	
	UFUNCTION(BlueprintCallable)
	FText GetCurrentName();
	
	
	UFUNCTION(BlueprintCallable)
	void Next();

	UFUNCTION(BlueprintCallable)
	void EndDialogue();
	
	UFUNCTION(BlueprintCallable)
	void ChooseChoice(const FDialogueChoice& Choice);
};
