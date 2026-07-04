// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "MonsterWaveRow.h"
#include "GameFramework/Actor.h"
#include "MonsterWaveSpawner.generated.h"

UCLASS()
class SNTP_API AMonsterWaveSpawner : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMonsterWaveSpawner();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDataTable* WaveTable;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoStart = false;
	
	UFUNCTION()
	void StartWaves();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	int32 CurrentWave = 0;
	
	TArray<FMonsterWaveRow*> CachedWaves;
	
	FTimerHandle WaveTimer;
	
	void SpawnWave(int32 WaveIndex);
	
	void SpawnMonsterGroup(const FMonsterSpawnInfo& Info);
	
};
