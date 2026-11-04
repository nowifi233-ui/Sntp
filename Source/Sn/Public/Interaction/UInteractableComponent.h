// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/DataTable.h"
#include "UInteractableComponent.generated.h"

class UInteractionAction;
class UUserWidget;

/**
 * One row in the global interaction data table (USnSettings). The row name is
 * the interactable's ID, e.g. "Chest_Legendary". Maps an ID to its display
 * text, behavior action and optional UI override.
 */
USTRUCT(BlueprintType)
struct FSnInteractionRow : public FTableRowBase
{
	GENERATED_BODY()

	/** Shown in the interaction prompt / pickup list. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FText DisplayName;

	/** Action executed when the player interacts (e.g. BP_Action_Open).
	 *  Hard reference on purpose: the DataTable cell editor doesn't support
	 *  TSoftClassPtr selection, and actions are tiny logic classes anyway. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TSubclassOf<UInteractionAction> ActionClass;

	/** UI override for this interaction (three-tier: action > row > PC default). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TSubclassOf<UUserWidget> OverrideWidgetClass;

	/** Config window opened when this interaction executes (e.g. a setup menu).
	 *  Injected into the action instance at execution; distinct from
	 *  OverrideWidgetClass (the interaction prompt). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	TSubclassOf<UUserWidget> ConfigWidgetClass;
};

/**
 * Data layer of the interaction system: mounted on interactable actors
 * (chests, doors, pickups). Holds the row ID that drives everything else -
 * the behavior and UI are resolved from the data table at runtime, so adding
 * an interactable type never touches C++.
 */
UCLASS(ClassGroup = (Custom), Blueprintable, meta = (BlueprintSpawnableComponent))
class SN_API UInteractableComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UInteractableComponent();

	//~ UActorComponent interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End of UActorComponent interface

	/** Row in the interaction data table (USnSettings) that defines this object. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FName InteractRowName;

	/** Pickups group into a list; other interactables show as a single prompt. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bIsPickup = false;

	/** False while the interactable is deactivated (e.g. a pickup resting in
	 *  the pool): excluded from the scan and the F-key check, so hidden
	 *  pickups can never be selected or interacted with. Replicated: on
	 *  replicated actors (item pickups) the server's enable/disable state
	 *  reaches clients, so their local scan skips the same actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = "Interaction")
	bool bEnabled = true;

	/** Persistent identifier for save games (persistence itself comes later). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	FString SaveID;
};
