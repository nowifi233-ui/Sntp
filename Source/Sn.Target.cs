// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SnTarget : TargetRules
{
	public SnTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// Same reason as SnEditor.Target.cs: VRM4ULoader has two .cpp files with
		// identical file-scope statics, which only collide under unity build.
		// Set on both targets so the module compiles the same way everywhere.
		DisableUnityBuildForModules = new string[] { "VRM4ULoader" };

		ExtraModuleNames.Add("Sn");
	}
}
