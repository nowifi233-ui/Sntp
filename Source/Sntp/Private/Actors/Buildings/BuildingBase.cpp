// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/BuildingBase.h"
#include "Components/BuildingComponent/BuildingSystemBlueprintLibrary.h"

// Sets default values
ABuildingBase::ABuildingBase()
{
	Health = 100.f;
	bCanRecall = true;
	Tags.Add(FName("Building"));
	
	BuildingMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("StaticMeshComponent");
}

void ABuildingBase::Recall(ACharacter* Player)
{
	Destroy();
}

void ABuildingBase::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	UStaticMesh* Mesh = UBuildingSystemBlueprintLibrary::GetBuildableMeshByTag(BuildingTag);
	BuildingMesh = Mesh;
	BuildingMeshComponent->SetStaticMesh(Mesh);
	
}

FGameplayTag ABuildingBase::GetBuildableTag()
{
	return BuildingTag;
}

UPrimitiveComponent* ABuildingBase::GetSocket() 
{
	return nullptr;
}
