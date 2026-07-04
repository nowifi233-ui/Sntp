#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "MonsterWaveRow.generated.h"

USTRUCT(BlueprintType)
struct FMonsterSpawnInfo
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<class ASntpEnemyCharacter> MonsterClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Count;
};

USTRUCT(BlueprintType)
struct FMonsterWaveRow : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 WaveIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DelayAfterWave = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FMonsterSpawnInfo> Monsters;
};