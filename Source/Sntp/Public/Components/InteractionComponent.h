// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"
class UCapsuleComponent;
class UWidgetComponent;
struct FInteractionOption;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowWidgetDelegate, const TArray<FInteractionOption>&, Options, FName, InteractalName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAddOptionDelegate, const FInteractionOption&, Option);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FHideWidgetDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOptionSelectedDelegate, const FInteractionOption&, Option);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionComponent();
	
	// OptionWidget To Show
	UPROPERTY(EditAnywhere)
	TObjectPtr<UWidgetComponent> OptionWidget;

	// Operator Delegates, Assign those delegates in widget controller.
	UPROPERTY(BlueprintAssignable)
	FShowWidgetDelegate ShowWidgetDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FHideWidgetDelegate HideWidgetDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FAddOptionDelegate AddOptionDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FAddOptionDelegate RemoveOptionDelegate;
	//~
	
protected:
	//~ AActor
	virtual void BeginPlay() override;
	// 
	
	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable;
	
	UPROPERTY()
	TArray<FInteractionOption> CurrentOptions;
	
	FTimerHandle ScanTimer;
	
	FOnOptionSelectedDelegate OnOptionSelected;
	
public:
	// Call it in controller to Interact option.
	void Interact(FInteractionOption Option);
	
	// Please call this function in Begin play. 
	void InitSphere(UCapsuleComponent* InSphereComponent);
	
private:
	// Set the Current interactable object, 
	void SetCurrentInteractable(TObjectPtr<AActor> Actor);
	
	//~ Update Objects in ranges, when overlap and end overlap. every second update object in range.
	UFUNCTION()
	void UpdateCurrentInteractable();
	
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UFUNCTION()
	void CurrentActorStateChanged();
	//~ End update object.
	
	//~ Tools 
	static bool AreOptionsEqual(const TArray<FInteractionOption>& A, const TArray<FInteractionOption>& B);
	
	UPROPERTY()
	TObjectPtr<UCapsuleComponent> DetectionSphere;
	//~ Tools End
};
