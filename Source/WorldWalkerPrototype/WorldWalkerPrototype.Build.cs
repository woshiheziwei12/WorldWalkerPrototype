using UnrealBuildTool;

public class WorldWalkerPrototype : ModuleRules
{
	public WorldWalkerPrototype(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PrivateIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"Niagara",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Json",
			"Slate",
			"SlateCore"
		});
	}
}
