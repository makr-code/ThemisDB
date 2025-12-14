// Copyright ThemisDB Team. Licensed under MIT License.

using UnrealBuildTool;

public class ThemisGISViewer : ModuleRules
{
	public ThemisGISViewer(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"Niagara",
			"ChaosVehicles",
			"GeometryCollectionEngine",
			"FieldSystemEngine"
		});

		PrivateDependencyModuleNames.AddRange(new string[] 
		{ 
			"Slate", 
			"SlateCore",
			"Json",
			"JsonUtilities",
			"HTTP"
		});

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
