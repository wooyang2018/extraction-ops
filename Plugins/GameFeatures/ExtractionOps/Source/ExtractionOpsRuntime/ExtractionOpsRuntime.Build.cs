// Copyright Extraction Ops. All Rights Reserved.

using UnrealBuildTool;

public class ExtractionOpsRuntime : ModuleRules
{
	public ExtractionOpsRuntime(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"ModularGameplay"
			});

		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"GameplayAbilities",
				"GameplayTags",
				"GameplayMessageRuntime",
				"LyraGame",
				"NetCore",
				"UMG"
			});
	}
}
