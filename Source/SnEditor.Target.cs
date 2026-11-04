// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class SnEditorTarget : TargetRules
{
	public SnEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;

		// VRM4U: VrmConvertModel.cpp and VrmConvertModel_Description.cpp both
		// define the same file-scope statics (VRM4U_InvMaxRawBoneWeightFloat,
		// BoneMapOpt, GetNodeFromMeshID, ...). Legal in separate translation
		// units, a hard redefinition error once unity merges them into one.
		// The plugin's own Build.cs has this switch but leaves it commented out,
		// and names a property that does not exist in 5.8 (bUseUnityBuild; the
		// real one is bUseUnity) - so reflection finds nothing and the fix is
		// silently skipped. Doing it here keeps it out of the third-party tree,
		// where a plugin update would drop it.
		DisableUnityBuildForModules = new string[] { "VRM4ULoader" };

		ExtraModuleNames.Add("Sn");
	}
}
