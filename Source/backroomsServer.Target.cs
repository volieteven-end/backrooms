using UnrealBuildTool;
public class backroomsServerTarget : TargetRules
{
    public backroomsServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("backrooms");
    }
}
