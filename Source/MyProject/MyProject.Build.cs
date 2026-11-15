using UnrealBuildTool;

public class MyProject : ModuleRules
{
    public MyProject(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new[] { "Core", "CoreUObject", "Engine", "UMG", "SlateCore", "MediaAssets" });
        PrivateDependencyModuleNames.AddRange(new[] { "InputCore", "Slate", "AudioMixer", "AnimGraphRuntime", "CinematicCamera" });
        if (Target.bBuildEditor)
        {
            PrivateDependencyModuleNames.AddRange(new[] { "UnrealEd", "IKRig", "IKRigEditor", "AssetRegistry", "Json", "MeshDescription", "StaticMeshDescription", "AnimGraph", "BlueprintGraph", "KismetCompiler", "MaterialEditor" });
        }
    }
}
