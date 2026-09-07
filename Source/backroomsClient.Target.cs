using UnrealBuildTool;
public class backroomsClientTarget : TargetRules
{
    public backroomsClientTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Client;
        DefaultBuildSettings = BuildSettingsVersion.V7;
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;
        ExtraModuleNames.Add("backrooms");
    }
}
