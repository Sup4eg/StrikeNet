// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Blaster : ModuleRules
{
	public Blaster(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.NoSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] {
		 });

		PublicIncludePaths.AddRange(new string[] {
			"Blaster/Public/Character",
			"Blaster/Public/GameMode",
			"Blaster/Public/HUD",
			"Blaster/Public/Weapon",
			"Blaster/Public/BlasterComponents",
			"Blaster/Public/BlasterTypes",
			"Blaster/Public/PlayerController",
			"Blaster/Public/Interfaces",
			"Blaster/Public/PlayerState"
		});
	}
}
