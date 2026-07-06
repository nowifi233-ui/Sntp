// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/Buildings/BuildingBase.h"
#include "MonsterPool/MonsterWaveRow.h"
#include "MonstPoolBuilding.generated.h"

struct FMonsterWaveRow;
/**
 * 
 */
UCLASS()
class SNTP_API AMonstPoolBuilding : public ABuildingBase, public IInteractable
{
	GENERATED_BODY()
	
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override {return BuildingName;}
	virtual EInteractionType GetType() override {return EInteractionType::Building;}
	FStateChangeDelegate StateChangeDelegate;
	
private:
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> Options;
	
public:
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UDataTable* WaveTable;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAutoStart = false;
	
	UFUNCTION()
	void StartWaves();
	
protected:
	
	int32 CurrentWave = 0;
	
	TArray<FMonsterWaveRow*> CachedWaves;
	
	FTimerHandle WaveTimer;
	
	void SpawnWave(int32 WaveIndex);
	
	void SpawnMonsterGroup(const FMonsterSpawnInfo& Info);
	
};
