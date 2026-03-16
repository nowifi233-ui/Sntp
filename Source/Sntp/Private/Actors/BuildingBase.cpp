// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/BuildingBase.h"

// Sets default values
ABuildingBase::ABuildingBase()
{
	Health = 100.f;
	bCanRecall = true;
}

void ABuildingBase::Recall(ACharacter* Player)
{
	Destroy();
}

TArray<FString> ABuildingBase::GetInteractionOptions()
{
	TArray<FString> Options;
	Options.Add("Inter");
	Options.Add("Recall");
	return Options;
}


