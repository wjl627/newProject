// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class newProject : ModuleRules
{
	public newProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"NavigationSystem",
			"UMG",
			"Slate",
				"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils", "Sockets" });

		PublicIncludePaths.AddRange(new string[] {
			"newProject",
			"newProject/Variant_Horror",
			"newProject/Variant_Horror/UI",
			"newProject/Variant_Shooter",
			"newProject/Variant_Shooter/AI",
			"newProject/Variant_Shooter/UI",
			"newProject/Variant_Shooter/Weapons"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
