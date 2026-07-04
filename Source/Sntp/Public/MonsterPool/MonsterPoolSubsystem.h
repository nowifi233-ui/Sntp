// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Characters/SntpEnemyCharacter.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MonsterPoolSubsystem.generated.h"

USTRUCT()
struct FMonsterPool
{
	GENERATED_BODY()
	
	UPROPERTY()
	TArray<TObjectPtr<ASntpEnemyCharacter>> Pool;
};
/**
 * 
 */
UCLASS()
class SNTP_API UMonsterPoolSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	ASntpEnemyCharacter* AcquireMonster(TSubclassOf<ASntpEnemyCharacter> MonsterClass, FVector Location);
	
	void ReleaseMonster(ASntpEnemyCharacter* Monster);

protected:
	//UPROPERTY()
	TMap<TSubclassOf<ASntpEnemyCharacter>, FMonsterPool> PoolMap;
	
	void PreSpawn(TSubclassOf<ASntpEnemyCharacter> MonsterClass, int32 Count);
};
