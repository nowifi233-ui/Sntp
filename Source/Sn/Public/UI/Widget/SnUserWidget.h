// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SnUserWidget.generated.h"

class USnWidgetController;

/**
 * Base class for widgets driven by a widget controller.
 * SetWidgetController fires WidgetControllerSet (bind callbacks in BP),
 * then pushes initial values so the widget is never empty on creation.
 */
UCLASS()
class SN_API USnUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Injects the data source. Any UObject works - a USnWidgetController
	 *  (GAS-driven) or any plain data source (e.g. a spawner actor with
	 *  delegates + getters). BP binds its delegates in WidgetControllerSet. */
	UFUNCTION(BlueprintCallable, Category = "WidgetController")
	void SetWidgetController(UObject* InController);

	UFUNCTION(BlueprintPure, Category = "WidgetController")
	UObject* GetWidgetController() const { return WidgetController; }

protected:
	/** C++ hook fired before the BP WidgetControllerSet event. Native subclasses
	 *  (e.g. attribute rows) resolve data and bind here. */
	virtual void OnWidgetControllerSet() {}

	/** BP override: bind to the data source's delegates here. */
	UFUNCTION(BlueprintImplementableEvent, Category = "WidgetController")
	void WidgetControllerSet();

	/** Escape closes the topmost window. All windows now live on
	 *  USnActivatableWindow (window manager) and do not inherit this; the hook
	 *  stays for any legacy USnUserWidget-based window and is harmless for
	 *  non-window widgets (they never receive focus). */
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	UPROPERTY(BlueprintReadOnly, Category = "WidgetController")
	TObjectPtr<UObject> WidgetController;
};
