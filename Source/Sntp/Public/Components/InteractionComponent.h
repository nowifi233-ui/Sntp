// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/SphereComponent.h"
#include "InteractionComponent.generated.h"
class UWidgetComponent;
struct FInteractionOption;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FShowWidgetDelegate, const TArray<FInteractionOption>&, Options, FName, InteractalName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNotShowWidgetDelegate);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOptionSelectedDelegate, int32, Index);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class SNTP_API UInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UInteractionComponent();
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<UWidgetComponent> OptionWidget;

	UPROPERTY(BlueprintAssignable)
	FShowWidgetDelegate ShowWidgetDelegate;
	
	UPROPERTY(BlueprintAssignable)
	FNotShowWidgetDelegate HideWidgetDelegate;
protected:
	virtual void BeginPlay() override;
	void FindInteractable();
	
	UPROPERTY()
	TObjectPtr<AActor> CurrentInteractable;
	
	UPROPERTY()
	TArray<FInteractionOption> CurrentOptions;
	
	FTimerHandle ScanTimer;
	
	UPROPERTY()
	TObjectPtr<USphereComponent> DetectionSphere;
	
	
	FOnOptionSelectedDelegate OnOptionSelected;
	
public:
	void Interact(int32 Index);
	void InitSphere(USphereComponent* InSphereComponent);
	
private:
	void SetCurrentInteractable(TObjectPtr<AActor> Actor);
	void UpdateCurrentInteractable();
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult & SweepResult);
	UFUNCTION()
	void OnEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	bool AreOptionsEqual(const TArray<FInteractionOption>& A, const TArray<FInteractionOption>& B);
};
