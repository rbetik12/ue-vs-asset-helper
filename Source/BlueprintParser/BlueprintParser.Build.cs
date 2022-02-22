// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class BlueprintParser : ModuleRules
{
	public BlueprintParser(ReadOnlyTargetRules Target) : base(Target)
	{
		// PublicDependencyModuleNames.AddRange();
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateDependencyModuleNames.AddRange(
		new []{
				"Core",
				"CoreUObject",
				"Engine",
				"Slate",
				"SlateCore",
				"BlueprintGraph",
				"AIModule",
				"Sockets",
				"Networking",
				"Json",
				"JsonUtilities",
				"UnrealEd"
			});
	}
}
