// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/BuildingComponent/BuildableManagerComponent.h"

#include "Camera/CameraComponent.h"
#include "Components/BuildingComponent/BuildableCharacterInterface.h"
#include "Components/BuildingComponent/BuildableData.h"
#include "Components/BuildingComponent/BuildingSystemBlueprintLibrary.h"
#include "Compression/lz4.h"
#include "Runtime/SlateCore/Private/Application/ActiveTimerHandle.h"

// Sets default values for this component's properties
UBuildableManagerComponent::UBuildableManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = false;

	// ...
}

void UBuildableManagerComponent::UpdateAllBuildableTags()
{
	AllBuildableTags.Reset();
	AllBuildableTags = UBuildingSystemBlueprintLibrary::GetAllBuildableTag();
	TryPrintAllBuildableTags();
}

void UBuildableManagerComponent::TryPrintAllBuildableTags()
{
	if (!EnableDebug) return;
	
	int32 DebugKey = 0;
	for (auto& Tag : AllBuildableTags)
	{
		GEngine->AddOnScreenDebugMessage(DebugKey, 3.f, FColor::Yellow, Tag.ToString());
		++DebugKey;
	}
}

void UBuildableManagerComponent::InitializeBuildPreview()
{
	PreviewStaticComponent = Cast<UStaticMeshComponent>(GetOwner()->AddComponentByClass(UStaticMeshComponent::StaticClass(), false, BuildPreviewTransform, false));
	PreviewStaticComponent->SetStaticMesh(GetBuildableDataByIndex(CurrentBuildPreviewIndex).StaticMesh);
	PreviewStaticComponent->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	PreviewStaticComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	UpdateBuildPreviewMeshByIndex(CurrentBuildPreviewIndex);
}

void UBuildableManagerComponent::InitialBuildableManagerComponent(UCameraComponent* InCameraComponent)
{
	OwnerCameraComponent = InCameraComponent;
}

FBuildableData UBuildableManagerComponent::GetBuildableDataByIndex(int32 Index)
{
	if (!AllBuildableTags.IsValidIndex(Index))
	{
		return FBuildableData();
	}
	
	FGameplayTag Tag = AllBuildableTags.GetByIndex(Index);
	FBuildableData Data = UBuildingSystemBlueprintLibrary::GetBuildableDataByTag(Tag);
	return Data;
}

void UBuildableManagerComponent::BuildPreviewTrace()
{
	FHitResult HitResult;
	TArray<AActor*> ActorsToIgnore;
	
	ActorsToIgnore.Add(GetOwner());
	FCollisionQueryParams Params;
	Params.bTraceComplex = false;
	
	Params.AddIgnoredActor(GetOwner());
	
	// FVector StartLocation = GetOwner()->GetActorLocation();
	FVector StartLocation = OwnerCameraComponent->GetComponentLocation();
	FVector EndLocation = OwnerCameraComponent->GetForwardVector() * BuildPreviewTraceDistance + StartLocation;
	ETraceTypeQuery TraceType = GetBuildableDataByIndex(CurrentBuildPreviewIndex).TraceChannel;
	ECollisionChannel CollisionChannel = UEngineTypes::ConvertToCollisionChannel(TraceType);
	
	bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		StartLocation,
		EndLocation,
		CollisionChannel,
		Params
	);
	
	if (EnableDebug)
	{
		DrawDebugLine(GetWorld(),StartLocation,EndLocation,FColor::Yellow,false,0.2f,0, 3.0f);
	}
	
	if (bHit)
	{
		UpdateBuildPreview(EBuildPreviewStatus::CanPlace);
		BuildPreviewTransform = FTransform(
			FRotator(0, 0, 0),
			HitResult.Location,
			FVector(1, 1, 1));
	}
	else
	{
		UpdateBuildPreview(EBuildPreviewStatus::CanNotPlace);
		BuildPreviewTransform = FTransform(
			FRotator(0, 0, 0),
			HitResult.TraceEnd,
			FVector(1, 1, 1));
	}
	
	SetBuildPreviewTransform();
}

void UBuildableManagerComponent::UpdateBuildPreview(EBuildPreviewStatus InBuildPreviewStatus)
{
	int32 MaterialNumber = PreviewStaticComponent->GetNumMaterials();
	UMaterialInterface* Material = CanBuildMaterial;
	switch (InBuildPreviewStatus)
	{
	case EBuildPreviewStatus::CanPlace:
		Material = CanBuildMaterial;
		break;
	case EBuildPreviewStatus::CanNotPlace:
		Material = CanNotBuildMaterial;
		break;
	default:
		break;
	}
	
	for (int i = 0; i < MaterialNumber; i++)
	{
		PreviewStaticComponent->SetMaterial(i, Material);
	}	
	
	BuildPreviewStatus = InBuildPreviewStatus;
}

void UBuildableManagerComponent::UpdateBuildPreviewMeshByIndex(int32 Index)
{
	FBuildableData Data = GetBuildableDataByIndex(Index);
	if (Data.StaticMesh != nullptr && PreviewStaticComponent)
	{
		PreviewStaticComponent->SetStaticMesh(Data.StaticMesh);
	}
}

void UBuildableManagerComponent::UpdateCurrentBuildPreviewIndex(EBuildPreviewUpdateIndex Method)
{
	switch (Method)
	{
	case EBuildPreviewUpdateIndex::Increment:
		CurrentBuildPreviewIndex++;
		if (CurrentBuildPreviewIndex >= GetAllBuildableTagsLength())
		{
			CurrentBuildPreviewIndex = 0;
		}
		UpdateBuildPreviewMeshByIndex(CurrentBuildPreviewIndex);
		break;
	case EBuildPreviewUpdateIndex::Decrement:
		CurrentBuildPreviewIndex--;
		if (CurrentBuildPreviewIndex < 0)
		{
			CurrentBuildPreviewIndex = GetAllBuildableTagsLength() - 1;
		}
		UpdateBuildPreviewMeshByIndex(CurrentBuildPreviewIndex);
		break;
	case EBuildPreviewUpdateIndex::First:
		CurrentBuildPreviewIndex = 0;
		UpdateBuildPreviewMeshByIndex(CurrentBuildPreviewIndex);
		break;
	case EBuildPreviewUpdateIndex::Last:
		CurrentBuildPreviewIndex = GetAllBuildableTagsLength();
		UpdateBuildPreviewMeshByIndex(CurrentBuildPreviewIndex);
		break;
	default:
		break;
	}
}

void UBuildableManagerComponent::TryPlaceBuildable(const int32 Index)
{
	if (!IsBuildModeActive)
	{
		return;
	}
	if (BuildPreviewStatus != EBuildPreviewStatus::CanPlace)
	{
		return;
	}
	
	FBuildableData Data = GetBuildableDataByIndex(Index);
	
	FActorSpawnParameters SpawnParameters;
	// SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	GetWorld()->SpawnActor<AActor>(
		Data.Class,
		BuildPreviewTransform,
		SpawnParameters
	);
}

void UBuildableManagerComponent::ActivateBuildMode()
{
	IsBuildModeActive = true;
	InitializeBuildPreview();
	ActivateBuildPreviewTrace();
}

void UBuildableManagerComponent::DeactivateBuildMode()
{
	IsBuildModeActive = false;
	if (BuildPreviewTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(BuildPreviewTimerHandle);
	}
	IBuildableCharacterInterface::Execute_DestroyBuildableComponent(GetOwner(), PreviewStaticComponent);
}

void UBuildableManagerComponent::ToggleBuildMode()
{
	if (IsBuildModeActive)
	{
		DeactivateBuildMode();
	}
	else
	{
		ActivateBuildMode();
	}
}

void UBuildableManagerComponent::ActivateBuildPreviewTrace()
{
	GetWorld()->GetTimerManager().SetTimer(
		BuildPreviewTimerHandle,
		this,
		&UBuildableManagerComponent::BuildPreviewTrace,
		BuildPreviewTickRate,
		true
		);
}


// Called when the game starts
void UBuildableManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	UpdateAllBuildableTags();
}

void UBuildableManagerComponent::SetBuildPreviewTransform()
{
	PreviewStaticComponent->SetWorldTransform(BuildPreviewTransform);
}

int32 UBuildableManagerComponent::GetAllBuildableTagsLength()
{
	return AllBuildableTags.Num();
}


