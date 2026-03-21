// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/Buildings/FieldPlot.h"

#include "UObject/FastReferenceCollector.h"

// Sets default values
AFieldPlot::AFieldPlot()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	
}

TArray<FInteractionOption> AFieldPlot::GetInteractionOptions()
{
	TArray<FInteractionOption> Result;

	switch (FieldPlotStatus)
	{
	case EFieldPlotStatus::Empty:
		Result = EmptyOptions;
		break;

	case EFieldPlotStatus::Planting:
		Result = PlantingOptions;
		break;

	case EFieldPlotStatus::Mature:
		Result = MatureOptions;
		break;
	}
	for (auto& Option : Result)
	{
		Option.SourceActor = this;
	}

	return Result;
}

void AFieldPlot::Interact(AActor* Interactor, FName OptionName)
{
	if (OptionName == "Plant")
	{
		FieldPlotStatus = EFieldPlotStatus::Planting;
		RemainingTime = Duration;
		StateChangeDelegate.Broadcast();
	}
	if (OptionName == "Water")
	{
		// TODO: 开始计时
		GetWorldTimerManager().SetTimer(
			GrowthTimer,
			this,
			&ThisClass::GrowthTimerHandler,
			1,
			true
			);
		CreateTimerWidget();
	}
	if (OptionName == "Harvest")
	{
		FieldPlotStatus = EFieldPlotStatus::Empty;
		StateChangeDelegate.Broadcast();
		// TODO: 给玩家物品
	}
}

void AFieldPlot::GrowthTimerHandler()
{
	RemainingTime -= 1;
	GrowthTimeChanged.Broadcast(RemainingTime);
	if (RemainingTime <= 0 && FieldPlotStatus != EFieldPlotStatus::Mature)
	{
		GetWorldTimerManager().ClearTimer(GrowthTimer);
		RemainingTime = 0;
		
		FieldPlotStatus = EFieldPlotStatus::Mature;
		StateChangeDelegate.Broadcast();
		GrowthTimeChanged.Broadcast(0);
		
		if (TimerWidgetComponent != nullptr)
		{
			TimerWidgetComponent->DestroyComponent();
			TimerWidgetComponent = nullptr;
		}
	}
}

void AFieldPlot::CreateTimerWidget()
{
	if (TimerWidgetComponent != nullptr) return;
	TimerWidgetComponent = NewObject<UWidgetComponent>(this);
	TimerWidgetComponent->RegisterComponent(); // 🔥必须
	TimerWidgetComponent->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

	TimerWidgetComponent->SetWidgetSpace(EWidgetSpace::Screen);
	TimerWidgetComponent->SetDrawSize(FVector2D(200, 100));
	TimerWidgetComponent->SetRelativeLocation(FVector(0, 0, 100));
	
	TimerWidgetComponent->SetWidgetClass(TimerWidgetClass);
	
	USntpUserWidget* TimerWidget = Cast<USntpUserWidget>(TimerWidgetComponent->GetUserWidgetObject());
	if (TimerWidget != nullptr)
	{
		TimerWidget->SetWidgetController(this);
		GrowthTimeChanged.Broadcast(RemainingTime);
	}
}



