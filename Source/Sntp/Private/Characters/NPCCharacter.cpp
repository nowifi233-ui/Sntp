// Fill out your copyright notice in the Description page of Project Settings.


#include "Characters/NPCCharacter.h"

// Sets default values
ANPCCharacter::ANPCCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

	DialogueComponent = CreateDefaultSubobject<UDialogueComponent>("DialogueComponent");
}

// Called when the game starts or when spawned
void ANPCCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called to bind functionality to input
void ANPCCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

}

TArray<FInteractionOption> ANPCCharacter::GetInteractionOptions()
{
	for (auto& Option : Options)
	{
		Option.SourceActor = this;
	}
	return Options;
}

void ANPCCharacter::Interact(AActor* Interactor, FName OptionName)
{
	Blueprint_Interact(Interactor, OptionName);
}



