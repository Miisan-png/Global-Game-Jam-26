// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GGJ26 : ModuleRules
{
	public GGJ26(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });
	}
}
