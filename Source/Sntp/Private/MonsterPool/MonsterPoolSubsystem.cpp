// Fill out your copyright notice in the Description page of Project Settings.


#include "MonsterPool/MonsterPoolSubsystem.h"

void UMonsterPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

ASntpEnemyCharacter* UMonsterPoolSubsystem::AcquireMonster(TSubclassOf<ASntpEnemyCharacter> MonsterClass,
                                                           FVector Location)
{
	auto& Pool = PoolMap.FindOrAdd(MonsterClass).Pool;
	for (ASntpEnemyCharacter* Monster : Pool)
	{
		if (Monster && !Monster->IsActive())
		{
			Monster->ActivateMonster(Location);
			return Monster;
		}
	}
	
	// 不够
	UWorld* World = GetWorld();
	if (!World) return nullptr;
	
	ASntpEnemyCharacter* NewMonster = World->SpawnActor<ASntpEnemyCharacter>(MonsterClass, Location, FRotator::ZeroRotator);
	Pool.Add(NewMonster);
	return NewMonster;
}

void UMonsterPoolSubsystem::ReleaseMonster(ASntpEnemyCharacter* Monster)
{
	if (Monster)
	{
		Monster->DeactivateMonster();
	}
}

void UMonsterPoolSubsystem::PreSpawn(TSubclassOf<ASntpEnemyCharacter> MonsterClass, int32 Count)
{
	if (!MonsterClass) return;
	
	UWorld* World = GetWorld();
	if (!World) return;
	
	for (int32 i = 0; i < Count; ++i)
	{
		ASntpEnemyCharacter* Monster = World->SpawnActor<ASntpEnemyCharacter>(MonsterClass, FVector::ZeroVector, FRotator::ZeroRotator);
		Monster->DeactivateMonster();
		PoolMap.FindOrAdd(MonsterClass).Pool.Add(Monster);
	}
}
