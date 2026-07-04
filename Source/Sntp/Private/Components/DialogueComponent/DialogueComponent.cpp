// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/DialogueComponent/DialogueComponent.h"

// Sets default values for this component's properties
UDialogueComponent::UDialogueComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

}


// Called when the game starts
void UDialogueComponent::BeginPlay()
{
	Super::BeginPlay();
	
}

void UDialogueComponent::StartDialogue(UDialogueDataAsset* Data)
{
	CurrentDialogue = Data;
	CurrentID = "1";
	ShowCurrentLine();
	
}

void UDialogueComponent::ShowCurrentLine()
{
	DialogueDelegate.Broadcast(GetCurrentName(), GetCurrentLine(), CurrentDialogue->GetNodeByID(CurrentID).Choices);
}

FText UDialogueComponent::GetCurrentLine()
{
	if (CurrentDialogue->GetNodeByID(CurrentID) == FDialogueNode())
	{
		CurrentID = "1";
		return FText();
	}
	return CurrentDialogue->GetNodeByID(CurrentID).Content;
}

FText UDialogueComponent::GetCurrentName()
{
	if (CurrentDialogue->GetNodeByID(CurrentID) == FDialogueNode())
	{
		CurrentID = "1";
		return FText();
	}
	return CurrentDialogue->GetNodeByID(CurrentID).Speaker;
}

void UDialogueComponent::Next()
{
	if (!CurrentDialogue->GetNodeByID(CurrentID).Choices.IsEmpty())
	{
		// Has choice
		return;
	}
	CurrentID = CurrentDialogue->GetNodeByID(CurrentID).NextID;
	if (CurrentDialogue->GetNodeByID(CurrentID) == FDialogueNode())
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

void UDialogueComponent::ChooseChoice(const FDialogueChoice& Choice)
{
	CurrentID = Choice.NextNodeID;
	if (CurrentDialogue->GetNodeByID(CurrentID) == FDialogueNode())
	{
		EndDialogue();
		return;
	}
	ShowCurrentLine();
}



