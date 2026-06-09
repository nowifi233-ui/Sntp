// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/DialogueComponent/DialogueComponent.h"

// Sets default values for this component's properties
UDialogueComponent::UDialogueComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}


// Called when the game starts
void UDialogueComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UDialogueComponent::StartDialogue(UDialogueDataAsset* Data)
{
	CurrentDialogue = Data;
	CurrentIndex = 0;
	ShowCurrentLine();
	
}

void UDialogueComponent::ShowCurrentLine()
{
	DialogueDelegate.Broadcast(GetCurrentName(), GetCurrentLine());
}

FText UDialogueComponent::GetCurrentLine()
{
	if (CurrentIndex >= CurrentDialogue->Lines.Num())
	{
		CurrentIndex = 0;
		return FText();
	}
	return CurrentDialogue->Lines[CurrentIndex].Context;
}

FText UDialogueComponent::GetCurrentName()
{
	if (CurrentIndex >= CurrentDialogue->Lines.Num())
	{
		CurrentIndex = 0;
		return FText();
	}
	return CurrentDialogue->Lines[CurrentIndex].SpeakerName;
}

void UDialogueComponent::Next()
{
	CurrentIndex++;
	if (CurrentIndex >= CurrentDialogue->Lines.Num())
	{
		EndDialogue();
		return;
	}
	ShowCurrentLine();
	
}

void UDialogueComponent::EndDialogue()
{
	StopDialogue.Broadcast();
}



