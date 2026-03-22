// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuildingBase.h"
#include "GameFramework/Actor.h"
#include "Interaction/Interactable.h"
#include "UI/Widgets/SntpUserWidget.h"
#include "FieldPlot.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGrowthTimeChanged, int32, RemainingTime);

UENUM(Blueprintable, BlueprintType)
enum class EFieldPlotStatus : uint8
{
	Empty,
	Planting,
	Mature
};


UCLASS(Blueprintable, BlueprintType)
class SNTP_API AFieldPlot : public ABuildingBase, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFieldPlot();
	
	UFUNCTION(BlueprintCallable)
	virtual TArray<FInteractionOption> GetInteractionOptions() override;
	virtual void Interact(AActor* Interactor, FName OptionName) override;
	virtual FName GetInteractionName() override {return BuildingName;}
	virtual EInteractionType GetType() override {return EInteractionType::Building;}
	
	UPROPERTY(EditAnywhere)
	int32 Duration = 3000;
	
	UPROPERTY(BlueprintAssignable)
	FGrowthTimeChanged GrowthTimeChanged;;
	
	UPROPERTY()
	UWidgetComponent* TimerWidgetComponent;
	
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<USntpUserWidget> TimerWidgetClass;
private:
	EFieldPlotStatus FieldPlotStatus;
	/*
	 * Options of Selecting
	 */
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> EmptyOptions;
	
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> PlantingOptions;
	
	UPROPERTY(EditAnywhere)
	TArray<FInteractionOption> MatureOptions;
	
	/**
	 *  Mesh
	 */
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* StaticMeshComponent;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* SeedMesh;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* GrowthMesh;
	
	UPROPERTY(EditDefaultsOnly)
	UStaticMesh* MatureMesh;
	
	void UpdateCropMesh();
	
	/*
	 * AActor Result
	 */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> HarvestActor;

	/**
	 *  Timer
	 */
	
	FTimerHandle GrowthTimer;
	int32 RemainingTime;
	
	UFUNCTION()
	void GrowthTimerHandler();
	
	UFUNCTION()
	void CreateTimerWidget();
	
};
