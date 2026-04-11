// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "BuildableData.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Runtime/SlateCore/Private/Application/ActiveTimerHandle.h"
#include "BuildableManagerComponent.generated.h"


UENUM(BlueprintType)
enum class EBuildPreviewStatus : uint8
{
	None,
	CanPlace,
	CanNotPlace
};

UENUM(BlueprintType)
enum class EBuildPreviewUpdateIndex : uint8
{
	None,
	Increment,
	Decrement,
	First,
	Last
};

class UCameraComponent;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UBuildableManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UBuildableManagerComponent();

	UFUNCTION(BlueprintCallable)
	void UpdateAllBuildableTags();
	
	UFUNCTION(BlueprintCallable)
	void TryPrintAllBuildableTags();
	
	UFUNCTION(BlueprintCallable)
	void InitializeBuildPreview();
	
	void InitialBuildableManagerComponent(UCameraComponent* InCameraComponent);
	
	FBuildableData GetBuildableDataByIndex(int32 Index);
	

	/**
	 * Set Preview Materials in Blueprint
	 */
	void BuildPreviewTrace();
	
	UFUNCTION(BlueprintCallable)
	void UpdateBuildPreview(EBuildPreviewStatus InBuildPreviewStatus);
	
	UFUNCTION(BlueprintCallable)
	void UpdateBuildPreviewMeshByIndex(int32 Index);
	
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* CanBuildMaterial;
	
	UPROPERTY(EditDefaultsOnly)
	UMaterialInterface* CanNotBuildMaterial;
	
	UFUNCTION(BlueprintCallable)
	void UpdateCurrentBuildPreviewIndex(EBuildPreviewUpdateIndex Method);
	
	UPROPERTY(BlueprintReadOnly)
	int32 CurrentBuildPreviewIndex = 0;
	
	/**
	 * Place
	 */
	UFUNCTION(BlueprintCallable)
	void TryPlaceBuildable(int32 Index);
	
	// Events
	UFUNCTION(BlueprintCallable)
	void ActivateBuildMode();
	
	UFUNCTION(BlueprintCallable)
	void DeactivateBuildMode();
	
	UFUNCTION(BlueprintCallable)
	void ToggleBuildMode();
	
	UFUNCTION(BlueprintCallable)
	void ActivateBuildPreviewTrace();
	
public:
	
	UPROPERTY(BlueprintReadOnly)
	bool IsBuildModeActive = false;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	bool EnableDebug = false;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	float BuildPreviewTickRate = 0.02f;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	float BuildPreviewTraceDistance = 1200.f;
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	
private:
	UPROPERTY()
	FGameplayTagContainer AllBuildableTags;
	
	FTransform BuildPreviewTransform;
	
	UPROPERTY()
	UStaticMeshComponent* PreviewStaticComponent;
	
	FTimerHandle BuildPreviewTimerHandle;
	
	UPROPERTY()
	UCameraComponent* OwnerCameraComponent;
	
	void SetBuildPreviewTransform();
	int32 GetAllBuildableTagsLength();
	EBuildPreviewStatus BuildPreviewStatus;
};
