#include "Misc/AutomationTest.h"
#include "Audio/BRGameplayAudioSettings.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRGameplayAudioAssets,"Backrooms.Audio.AssetBindings",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBRGameplayAudioAssets::RunTest(const FString&)
{
    const auto* Settings=GetDefault<UBRGameplayAudioSettings>();
    const int32 EventCount = static_cast<int32>(EBRGameplaySound::SkinStealerRoar) + 1;
    TestEqual(TEXT("All parking audio bindings"),Settings->Sounds.Num(),EventCount);
    TestFalse(TEXT("Discovery roar is a one-shot, not an overlapping loop"),UBRGameplayAudioSettings::IsLoop(EBRGameplaySound::SkinStealerRoar));
    TestTrue(TEXT("Roar interval avoids per-frame spam"),Settings->RoarInterval>=2.f);
    for (int32 I=0;I<EventCount;++I)
    {
        const auto* Entry=Settings->Sounds.Find(static_cast<EBRGameplaySound>(I));
        if (!TestNotNull(TEXT("Entry exists"),Entry)) continue;
        USoundWave* Wave=Entry->Sound.LoadSynchronous();
        if (TestNotNull(*Entry->Sound.ToString(),Wave)) TestTrue(TEXT("Non-empty sound"),Wave->Duration>0);
        TestTrue(TEXT("Volume is bounded"),Entry->Volume>=0 && Entry->Volume<=2);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRGameplayAudioLoop,"Backrooms.Audio.LoopDoesNotMutateAsset",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBRGameplayAudioLoop::RunTest(const FString&)
{
    USoundWave* Wave=GetDefault<UBRGameplayAudioSettings>()->Sounds.FindChecked(EBRGameplaySound::Ambience).Sound.LoadSynchronous();
    if (!TestNotNull(TEXT("Ambience wave"),Wave)) return false;
    const bool Before=Wave->bLooping;
    const bool DirtyBefore=Wave->GetOutermost()->IsDirty();
    USoundCue* Cue=UBRGameplayAudioSubsystem::BuildLoopCue(GetTransientPackage(),Wave);
    TestNotNull(TEXT("Transient cue"),Cue);
    if (Cue)
    {
        auto* Node=Cast<USoundNodeWavePlayer>(Cue->FirstNode);
        TestTrue(TEXT("Loop enabled on transient node"),Node && Node->bLooping);
        TestTrue(TEXT("Original waveform reused"),Node && Node->GetSoundWave()==Wave);
    }
    TestEqual(TEXT("Original loop flag unchanged"),bool(Wave->bLooping),Before);
    TestEqual(TEXT("Original dirty flag unchanged"),Wave->GetOutermost()->IsDirty(),DirtyBefore);
    return true;
}
#endif
