// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/FishingComponent/DialogueDataAsset.h"
#include "DialogueComponent.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FStopDialogueDelegate);

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
	UDialogueDataAsset* CurrentDialogue;
	
	UPROPERTY(BlueprintAssignable)
	FStopDialogueDelegate StopDialogue;
	
public:
	UFUNCTION(BlueprintCallable)
	void StartDialogue(UDialogueDataAsset* Data);
	
	UFUNCTION(BlueprintCallable)
	void ShowCurrentLine();
	
	UFUNCTION(BlueprintCallable)
	FText GetCurrentLine();
	
	UFUNCTION(BlueprintCallable)
	void Next();

	UFUNCTION(BlueprintCallable)
	void EndDialogue();
};
