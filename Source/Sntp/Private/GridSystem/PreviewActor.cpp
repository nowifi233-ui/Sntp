// Fill out your copyright notice in the Description page of Project Settings.


#include "GridSystem/PreviewActor.h"

// Sets default values
APreviewActor::APreviewActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
	Root = CreateDefaultSubobject<USceneComponent>("Root");
	RootComponent = Root;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	Mesh->SetupAttachment(RootComponent);
}

void APreviewActor::SetValid(bool bValid)
{
	if (!DynamicMaterial)
	{
		DynamicMaterial = Mesh->CreateDynamicMaterialInstance(0);
	}
	if (DynamicMaterial)
	{
		if (bValid)
		{
			DynamicMaterial->SetVectorParameterValue("Color", FLinearColor::Green);
		}
		else
		{
			DynamicMaterial->SetVectorParameterValue("Color", FLinearColor::Red);
		}
	}
}


