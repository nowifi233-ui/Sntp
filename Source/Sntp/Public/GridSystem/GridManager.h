// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GridManager.generated.h"


USTRUCT(BlueprintType)
struct FGridCell
{
	GENERATED_BODY()
	
public:
	UPROPERTY()
	bool bOccupied = false;
	
	UPROPERTY()
	AActor* Occupier = nullptr;
};



UCLASS()
class SNTP_API AGridManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AGridManager();

protected:
	virtual void BeginPlay() override;
	
public:
	UPROPERTY(EditAnywhere, Category="Grid")
	int32 Width = 10;
	
	UPROPERTY(EditAnywhere, Category="Grid")
	int32 Height = 10;
	
	UPROPERTY(EditAnywhere, Category="Grid")
	float CellSize = 100.f;
	
	static AGridManager* Get(UWorld* World)
	{
		return Cast<AGridManager>(UGameplayStatics::GetActorOfClass(World, AGridManager::StaticClass()));
	}
	
private:
	UPROPERTY()
	TArray<FGridCell> Grid;
	
public:
	bool IsValidCoord(FIntPoint Coord) const;
	int32 GetIndex(FIntPoint Coord) const;
	
	FGridCell* GetCell(FIntPoint Coord);
	
	// Coord
	FIntPoint WorldToGrid(FVector Location) const;
	FVector GridToWorld(FIntPoint Coord) const;
	
	// Build
	bool CanPlace(FIntPoint Coord) const;
	
	AActor* Place(FIntPoint Coord, TSubclassOf<AActor> ActorClass) ;
	void Remove(FIntPoint Coord);
};
