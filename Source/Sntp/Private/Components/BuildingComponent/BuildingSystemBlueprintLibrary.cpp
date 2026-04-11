// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BuildingComponent/BuildingSystemBlueprintLibrary.h"

#include "GameplayTagContainer.h"
#include "Components/BuildingComponent/BuildableData.h"

UDataTable* UBuildingSystemBlueprintLibrary::GetBuildableDataTable()
{
	static const FString DT_Path = TEXT("/Game/Blueprints/Data/DT_BuildableData.DT_BuildableData");
	UDataTable* DataTable = Cast<UDataTable>(StaticLoadObject(UDataTable::StaticClass(), nullptr, *DT_Path));
	return DataTable;
}

FBuildableData UBuildingSystemBlueprintLibrary::GetBuildableDataByTag(const FGameplayTag Tag)
{
	UDataTable* DataTable = GetBuildableDataTable();
	
	if (!DataTable)
	{
		return FBuildableData();
	}
	FBuildableData* Row = DataTable->FindRow<FBuildableData>(Tag.GetTagName(), TEXT("BuildingData"));
		
	if (Row)
	{
		return *Row;
	}
	return FBuildableData();
}

UStaticMesh* UBuildingSystemBlueprintLibrary::GetBuildableMeshByTag(FGameplayTag Tag)
{
	FBuildableData Data = GetBuildableDataByTag(Tag);
	UStaticMesh* StaticMesh = Data.StaticMesh;
	
	if (StaticMesh)
	{
		return StaticMesh;
	}
	return nullptr;
}

FGameplayTagContainer UBuildingSystemBlueprintLibrary::GetAllBuildableTag()
{
	FGameplayTagContainer Tags;
	const UDataTable* DataTable = GetBuildableDataTable();
	TArray<FBuildableData*> BuildableData;
	DataTable->GetAllRows<FBuildableData>(TEXT("AllBuildableTag"), BuildableData);
	for (FBuildableData* DataRow : BuildableData)
	{
		Tags.AddTag(DataRow->Tag);
	}
	return Tags;
}


