// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/WidgetController/DialogueWidgetController.h"

#include "UI/SntpHUD.h"

void UDialogueWidgetController::Init(UDialogueComponent* InDialogueComponent, APlayerController* InPlayerController)
{
	DialogueComponent = InDialogueComponent;
	PlayerController = InPlayerController;
	DialogueComponent->StopDialogue.AddDynamic(this, &UDialogueWidgetController::StopDialogue);
	DialogueComponent->DialogueDelegate.AddDynamic(this, &UDialogueWidgetController::OnDialogueChanged);
}

void UDialogueWidgetController::SetDialogueComponent(UDialogueComponent* InDialogueComponent)
{
	DialogueComponent = InDialogueComponent;
}

void UDialogueWidgetController::NextDialogue()
{
	DialogueComponent->Next();
}

void UDialogueWidgetController::DialogueOptionChosen(FDialogueChoice Choice)
{
	DialogueComponent->ChooseChoice(Choice);
}

void UDialogueWidgetController::StartDialogue()
{
	
}

void UDialogueWidgetController::StopDialogue()
{
	ASntpHUD* SntpHUD = Cast<ASntpHUD>(PlayerController->GetHUD());
	if (SntpHUD)
	{
		SntpHUD->ToggleDialogueWidget(PlayerController, DialogueComponent);
	}
}

void UDialogueWidgetController::OnDialogueChanged(FText Name, FText Text, TArray<FDialogueChoice> Choices)
{
	OnDialogueChangedDelegate.Broadcast(Name, Text, FChoiceList{Choices});
}
