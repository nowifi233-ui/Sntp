// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/BuildingBase.h"

// Sets default values
ABuildingBase::ABuildingBase()
{
	Health = 100.f;
	bCanRecall = true;
	Tags.Add(FName("Building"));
}

void ABuildingBase::Recall(ACharacter* Player)
{
	Destroy();
}
