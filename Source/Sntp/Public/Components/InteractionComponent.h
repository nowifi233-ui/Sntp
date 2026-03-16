// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"
class UWidgetComponent;
struct FInteractionOption;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FShowWidgetDelegate, const TArray<FInteractionOption>&, Options);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FNotShowWidgetDelegate);

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
	TObjectPtr<AActor> LastInteractable;
	
	FTimerHandle ScanTimer;
	
public:
	void Interact(FName OptionName);
	
private:
	void SetCurrentInteractable(TObjectPtr<AActor> Actor);
};
