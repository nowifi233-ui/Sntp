// Fill out your copyright notice in the Description page of Project Settings.


#include "GridSystem/GridManager.h"

#include "IPropertyTable.h"
#include "Components/InstancedStaticMeshComponent.h"

// Sets default values
AGridManager::AGridManager()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

void AGridManager::BeginPlay()
{
	Super::BeginPlay();
	Grid.SetNum(Width * Height);
	
	GridMesh = NewObject<UInstancedStaticMeshComponent>(this);
	GridMesh->RegisterComponent();
	GridMesh->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepWorldTransform);
	
	GridMesh->SetStaticMesh(GridPlaneMesh);
	GridMesh->SetMaterial(0, GridMaterial);
	for (int32 x = 0; x < Width; x++)
	{
		for (int32 y = 0; y < Height; y++)
		{
			FVector Location = GridToWorld(FIntPoint(x, y));

			FTransform Transform;
			Transform.SetLocation(Location);
			Transform.SetScale3D(FVector(CellSize / 100.f)); // 根据Mesh调整

			GridMesh->AddInstance(Transform);
		}
	}
}

void AGridManager::SetGridMeshVisible(bool bVisible)
{
	if (GridMesh)
	{
		GridMesh->SetVisibility(bVisible);
	}
}

bool AGridManager::IsValidCoord(FIntPoint Coord) const
{
	return Coord.X >= 0 && Coord.X < Width && Coord.Y >= 0 && Coord.Y < Height;
}

int32 AGridManager::GetIndex(FIntPoint Coord) const
{
	return Coord.Y * Width + Coord.X;
}

FGridCell* AGridManager::GetCell(FIntPoint Coord)
{
	if (!IsValidCoord(Coord)) return nullptr;
	
	return &Grid[GetIndex(Coord)];
}

FIntPoint AGridManager::WorldToGrid(FVector Location) const
{
	int32 X = FMath::FloorToInt(Location.X / CellSize);
	int32 Y = FMath::FloorToInt(Location.Y / CellSize);
	return FIntPoint(X, Y);
}

FVector AGridManager::GridToWorld(FIntPoint Coord) const
{
	return FVector(Coord.X * CellSize + CellSize * 0.5f, Coord.Y * CellSize + CellSize * 0.5f, 0.f);
}

bool AGridManager::CanPlace(FIntPoint Coord) const
{
	if (!IsValidCoord(Coord)) return false;
	return !Grid[GetIndex(Coord)].bOccupied;
}

AActor* AGridManager::Place(FIntPoint Coord, TSubclassOf<AActor> ActorClass) 
{
	if (!CanPlace(Coord)) return nullptr;
	FGridCell& Cell = Grid[GetIndex(Coord)];
	FVector WorldPos = GridToWorld(Coord);
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	Params.bNoFail = true; // 强制生成！永不失败！
	
	AActor* Actor = GetWorld()->SpawnActor<AActor>(ActorClass, WorldPos, FRotator::ZeroRotator, Params);
	if (Actor)
	{
		Cell.bOccupied = true;
		Cell.Occupier = Actor;
	}
	return Actor;

}

void AGridManager::Remove(FIntPoint Coord)
{
	if (!IsValidCoord(Coord)) return;
	FGridCell& Cell = Grid[GetIndex(Coord)];
	if (Cell.Occupier)
	{
		Cell.Occupier->Destroy();
	}
	Cell.bOccupied = false;
	Cell.Occupier = nullptr;
}




