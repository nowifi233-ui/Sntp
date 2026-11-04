// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/NoExportTypes.h"
#include "UInteractionAction.generated.h"

class APlayerController;
class UUserWidget;

/**
 * Where an interaction action's Execute runs.
 *
 * UI actions (open windows, menus) run on the client that pressed F - that is
 * the default. World-modifying actions (pickups: add to bag + retire the
 * pickup) must run on the SERVER: executed locally on a client they only
 * mutate the client's copies (ghost inventory, the pickup stays alive on the
 * server and replicates back), and every later Server RPC rejects them. The
 * scheduler (ASnPlayerController) reads this declaration and routes Server
 * actions through ServerExecuteAction; the action blueprint's logic stays
 * identical either way.
 */
UENUM(BlueprintType)
enum class ESnActionExecution : uint8
{
	/** Execute on the local (interacting) client - windows and other pure UI. */
	Client,

	/** Execute on the server (authority) - world modifications. */
	Server,
};

/**
 * A single interaction behavior. Subclass in blueprint (BP_Action_Open) and
 * override Execute; C++ never switches on interaction type. Stateless and
 * instantiated per execution, so it is fully reusable.
 */
UCLASS(Blueprintable, Abstract)
class SN_API UInteractionAction : public UObject
{
	GENERATED_BODY()

public:
	/** Runs the interaction on the target. Override in a blueprint subclass. */
	UFUNCTION(BlueprintNativeEvent, Category = "Interaction")
	void Execute(APlayerController* PC, AActor* Target);
	virtual void Execute_Implementation(APlayerController* PC, AActor* Target);

	/** Where Execute runs (Client by default - see ESnActionExecution). Set to
	 *  Server in the blueprint subclass defaults for world-modifying actions
	 *  (e.g. BP_Action_Pickup): the scheduler then routes the execution through
	 *  the server RPC instead of running it locally. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Action")
	ESnActionExecution Execution = ESnActionExecution::Client;

	/** Convenience for actions that open a config window: forwards to the
	 *  player controller's OpenConfigWindow (creates the widget, injects the
	 *  data source, pauses the game, shows the cursor). The widget class comes
	 *  from the interaction row (ConfigWidgetClass), injected at execution.
	 *  Close it from the window itself via ASnPlayerController::CloseConfigWindow. */
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void OpenConfigWindow(APlayerController* PC, UObject* DataSource);

	/** Config window widget class from the interaction row, injected by the
	 *  player controller at execution. Used by OpenConfigWindow. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaction")
	TSubclassOf<UUserWidget> ConfigWidgetClass;

	/** Optional UI override: highest tier of the three-tier widget lookup. */
	UPROPERTY(EditDefaultsOnly, Category = "Interaction", meta = (AllowedClasses = "/Script/UMG.UserWidget"))
	TSoftClassPtr<UUserWidget> OverrideWidgetClass;
};
