// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class SWidget;

/**
 * The travel loading screen (armed by USnGameInstance::SetupLoadingScreen).
 *
 * Returns a ready Slate widget for the engine's movie player, which is what
 * actually draws it: the map load blocks the game thread, and while that
 * happens only the movie player's own Slate loading thread keeps ticking
 * (DefaultGameMoviePlayer spins it up in PlayMovie, ~60fps, and ticks the Slate
 * application from there). A UMG widget could not do this job - UUserWidget
 * ticking is game-thread bound and would freeze mid-load.
 *
 * The class lives in the .cpp and only the factory is exported, the same shape
 * as the engine's FLoadingScreenAttributes::NewTestLoadingScreenWidget: Slate
 * headers stay out of module headers, and callers get a widget without knowing
 * what it is.
 *
 * NOT visible in PIE - the engine disables the movie player in the editor
 * (IsMoviePlayerEnabled() is false when GIsEditor) and hands out a null player
 * instead. Use Play > Standalone Game to see it.
 */
SN_API TSharedRef<SWidget> SnMakeLoadingScreenWidget(const FText& TitleText, const FText& LoadingText);
