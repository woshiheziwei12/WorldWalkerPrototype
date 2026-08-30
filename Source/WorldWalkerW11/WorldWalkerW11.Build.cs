using UnrealBuildTool;

public class WorldWalkerW11 : ModuleRules
{
	public WorldWalkerW11(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add(ModuleDirectory);
		PrivateIncludePaths.Add(ModuleDirectory + "/../WorldWalkerPrototype");

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"AIModule",
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"InputCore",
			"NetCore",
			"Paper2D",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Slate",
			"SlateCore",
			"WorldWalkerPrototype"
		});
	}
}
