// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterPool/MonsterWaveSpawner.h"

#include "MonsterPool/MonsterPoolSubsystem.h"

// Sets default values
AMonsterWaveSpawner::AMonsterWaveSpawner()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMonsterWaveSpawner::StartWaves()
{
	if (!WaveTable) return;
	
	TArray<FMonsterWaveRow*> Rows;
	WaveTable->GetAllRows("", Rows);
	CachedWaves = Rows;
	CurrentWave = 0;
	SpawnWave(CurrentWave);
}

// Called when the game starts or when spawned
void AMonsterWaveSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (bAutoStart)
	{
		StartWaves();
	}
}

void AMonsterWaveSpawner::SpawnWave(int32 WaveIndex)
{
	if (!CachedWaves.IsValidIndex(WaveIndex)) return;
	
	FMonsterWaveRow* Wave = CachedWaves[WaveIndex];
	if (!Wave) return;
	
	UGameInstance* GI = GetGameInstance();
	auto Pool = GI->GetSubsystem<UMonsterPoolSubsystem>();
	
	FVector BaseLoc = GetActorLocation();
	
	for (const FMonsterSpawnInfo& Info : Wave->Monsters)
	{
		for (int32 i = 0; i < Info.Count; i++)
		{
			FVector Offset = FVector(FMath::RandRange(-300, 300), FMath::RandRange(-300, 300), 0);
			Pool->AcquireMonster(Info.MonsterClass, BaseLoc + Offset);
		}
	}
	float Delay = Wave->DelayAfterWave;
	
	GetWorldTimerManager().SetTimer(WaveTimer, [this]()
	{
		CurrentWave++;
		SpawnWave(CurrentWave);
	}, Delay, false);
}

void AMonsterWaveSpawner::SpawnMonsterGroup(const FMonsterSpawnInfo& Info)
{
}



