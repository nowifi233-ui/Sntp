// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/Widget/SnClickRipple.h"

void USnClickRipple::PlayAt(const FVector2D& ViewportPosition)
{
	// Center the ripple on the given viewport position (same coordinate
	// space as GetMousePositionOnViewport). DesiredSize is the widget's own
	// logical size; give the root a fixed size in BP so it is ready on first click.
	const FVector2D Size = GetDesiredSize();
	SetPositionInViewport(ViewportPosition - Size * 0.5f, /*bRemoveDPIScale=*/false);

	OnPlayRipple();
}
