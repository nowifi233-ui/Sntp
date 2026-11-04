// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Sn : ModuleRules
{
	public Sn(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "GameplayAbilities", "GameplayTags", "GameplayTasks", "UMG", "DeveloperSettings", "Niagara", "AIModule", "NavigationSystem" });

		// LAN lobby: manual IOnlineSession calls (find/join/create) + the LAN
		// session blueprint result types. OnlineSubsystemNull is the platform
		// layer (beacon discovery + resolved connect string); Utils is pulled in
		// by the FBlueprintSessionResult/FindSessionsCallbackProxy types.
		PublicDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemNull", "OnlineSubsystemUtils" });

		// Slate/SlateCore: the loading screen widget (SSnLoadingScreen) - raw
		// Slate, because it is ticked by the engine movie player's loading
		// thread while the game thread is blocked in LoadMap. MoviePlayer is
		// that same integration (FLoadingScreenAttributes / IGameMoviePlayer).
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "MoviePlayer" });

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
