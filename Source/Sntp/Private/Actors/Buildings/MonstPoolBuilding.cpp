// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/MonstPoolBuilding.h"

#include "MonsterPool/MonsterPoolSubsystem.h"
#include "MonsterPool/MonsterWaveRow.h"

class UMonsterPoolSubsystem;

TArray<FInteractionOption> AMonstPoolBuilding::GetInteractionOptions()
{
	for (auto& Option : Options)
	{
		Option.SourceActor = this;
	}
	return Options;
}

void AMonstPoolBuilding::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == FName("Open"))
	{
		// Show Widget
		// Interactor->Player->Controller->HUD->ShowWidget
		// Widget->WidgetController
		StartWaves();
	}
	if (OptionName == FName("Recall"))
	{
		Destroy();
	}
}

void AMonstPoolBuilding::StartWaves()
{
	if (!WaveTable) return;
	
	TArray<FMonsterWaveRow*> Rows;
	WaveTable->GetAllRows("", Rows);
	CachedWaves = Rows;
	CurrentWave = 0;
	SpawnWave(CurrentWave);
}

void AMonstPoolBuilding::SpawnWave(int32 WaveIndex)
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

void AMonstPoolBuilding::SpawnMonsterGroup(const FMonsterSpawnInfo& Info)
{
}
