#include "Audio/BRGameplayAudioSubsystem.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Components/AudioComponent.h"
#include "Core/BRGameMode.h"
#include "Core/BRGameState.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/WorldSettings.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "Sound/SoundWave.h"
#include "TimerManager.h"
#include "World/BRExtractionZone.h"

bool UBRGameplayAudioSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
    return Type == EWorldType::Game || Type == EWorldType::PIE;
}

USoundCue* UBRGameplayAudioSubsystem::BuildLoopCue(UObject* Outer, USoundWave* Wave)
{
    if (!Wave) return nullptr;
    USoundCue* Cue = NewObject<USoundCue>(Outer,NAME_None,RF_Transient);
    USoundNodeWavePlayer* Node = NewObject<USoundNodeWavePlayer>(Cue,NAME_None,RF_Transient);
    Node->SetSoundWave(Wave); Node->bLooping = true;
    Cue->FirstNode = Node; Cue->VolumeMultiplier = 1.0f;
    Cue->VirtualizationMode = EVirtualizationMode::PlayWhenSilent;
    Cue->CacheAggregateValues();
    return Cue; // Never change bLooping on the shared imported SoundWave asset.
}

bool UBRGameplayAudioSubsystem::LogPlayback() const
{
    return GetDefault<UBRGameplayAudioSettings>()->bLogPlayback || FParse::Param(FCommandLine::Get(),TEXT("BRAudioLog"));
}

UAudioComponent* UBRGameplayAudioSubsystem::PlayEvent(EBRGameplaySound Event, FVector Location, AActor* AttachTo, float Pitch)
{
    UWorld* World = GetWorld();
    const UBRGameplayAudioSettings* Settings = GetDefault<UBRGameplayAudioSettings>();
    if (!World || World->GetNetMode() == NM_DedicatedServer || !Settings->bEnabled || Settings->MasterVolume <= 0) return nullptr;
    const FBRGameplaySoundEntry* Entry = Settings->Sounds.Find(Event);
    if (!Entry || Entry->Sound.IsNull() || Entry->Volume <= 0) return nullptr;
    USoundWave* Wave = Entry->Sound.LoadSynchronous();
    if (!Wave)
    { UE_LOG(LogTemp,Warning,TEXT("BR_AUDIO result=MISSING event=%d path=%s"),int32(Event),*Entry->Sound.ToString()); return nullptr; }
    const bool bLoop = UBRGameplayAudioSettings::IsLoop(Event);
    USoundBase* Sound = Wave;
    if (bLoop)
    {
        TObjectPtr<USoundCue>& Cue = LoopCues.FindOrAdd(Event);
        if (!Cue) Cue = BuildLoopCue(this,Wave);
        Sound = Cue;
    }
    AActor* Owner = IsValid(AttachTo) ? AttachTo : World->GetWorldSettings();
    UAudioComponent* Component = NewObject<UAudioComponent>(Owner,NAME_None,RF_Transient);
    Component->bAutoActivate = false;
    Component->bAutoDestroy = !bLoop;
    Component->bStopWhenOwnerDestroyed = true;
    Component->bAllowSpatialization = Entry->bSpatial;
    Component->bIsUISound = false;
    Component->SetSound(Sound);
    Component->SetVolumeMultiplier(FMath::Clamp(Settings->MasterVolume,0.f,1.f) * FMath::Clamp(Entry->Volume,0.f,2.f));
    Component->SetPitchMultiplier(FMath::Clamp(Pitch,0.5f,2.0f));
    if (Entry->bSpatial)
    {
        FSoundAttenuationSettings Attenuation;
        Attenuation.bAttenuate = true; Attenuation.bSpatialize = true;
        Attenuation.AttenuationShape = EAttenuationShape::Sphere;
        Attenuation.AttenuationShapeExtents = FVector(FMath::Max(0.f,Entry->InnerRadius),0,0);
        Attenuation.FalloffDistance = FMath::Max(1.f,Entry->FalloffDistance);
        Attenuation.bEnableOcclusion = Entry->bOcclusion;
        Attenuation.bUseComplexCollisionForOcclusion = true;
        Attenuation.OcclusionVolumeAttenuation = Settings->OccludedVolume;
        Attenuation.OcclusionLowPassFilterFrequency = Settings->OccludedLowPassHz;
        Attenuation.OcclusionInterpolationTime = 0.2f;
        Attenuation.bApplyNormalizationToStereoSounds = true;
        Component->AdjustAttenuation(Attenuation);
    }
    if (IsValid(AttachTo) && AttachTo->GetRootComponent())
    { Component->SetupAttachment(AttachTo->GetRootComponent()); Component->SetRelativeLocation(FVector::ZeroVector); }
    else Component->SetWorldLocation(Location);
    Component->RegisterComponentWithWorld(World);
    if (bLoop)
    {
        Loops.RemoveAll([](const TObjectPtr<UAudioComponent>& C){return !IsValid(C);});
        Loops.Add(Component);
    }
    Component->Play();
    if (LogPlayback()) UE_LOG(LogTemp,Display,TEXT("BR_AUDIO result=PLAY event=%s loop=%d spatial=%d playing=%d wave=%s net=%d"),
        *StaticEnum<EBRGameplaySound>()->GetNameStringByValue(int64(Event)),bLoop,Entry->bSpatial,Component->IsPlaying(),*Wave->GetPathName(),int32(World->GetNetMode()));
    return Component;
}

FVector UBRGameplayAudioSubsystem::GetElevatorLocation(bool bArrival) const
{
    if (!bArrival)
        for (TActorIterator<ABRExtractionZone> It(GetWorld());It;++It) return It->GetActorLocation();
    APlayerStart* First = nullptr;
    for (TActorIterator<APlayerStart> It(GetWorld());It;++It)
        if (!First || It->GetName() < First->GetName()) First = *It;
    return First ? First->GetActorLocation() : FVector::ZeroVector;
}

void UBRGameplayAudioSubsystem::OnWorldBeginPlay(UWorld& World)
{
    Super::OnWorldBeginPlay(World);
    const auto Mode = World.GetWorldSettings()->DefaultGameMode;
    bGameplayWorld = Mode && Mode->IsChildOf(ABRGameMode::StaticClass());
    if (!bGameplayWorld) return;
    if (World.GetNetMode() == NM_DedicatedServer)
    { if (LogPlayback()) UE_LOG(LogTemp,Display,TEXT("BR_AUDIO result=DEDICATED_SERVER_SKIPPED")); return; }
    Ambience = PlayEvent(EBRGameplaySound::Ambience,FVector::ZeroVector);
    World.GetTimerManager().SetTimer(IntroTimer,FTimerDelegate::CreateWeakLambda(this,[this]()
    {
        PlayEvent(EBRGameplaySound::ElevatorBell,GetElevatorLocation(true));
        PlayEvent(EBRGameplaySound::ElevatorDoorOpen,GetElevatorLocation(true));
    }),0.6f,false);
    if (auto* State = World.GetGameState<ABRGameState>()) HandleLevelPhase(State->GetLevelPhase());
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("BRAudioSmoke"))) StartSmoke();
#endif
}

void UBRGameplayAudioSubsystem::HandleLevelPhase(EBRLevelPhase Phase)
{
    if (!bGameplayWorld || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
    if (Phase == EBRLevelPhase::ExtractionReady && !bReadyBellPlayed)
    { bReadyBellPlayed=true; PlayEvent(EBRGameplaySound::ElevatorBell,GetElevatorLocation(false)); }
    if ((Phase == EBRLevelPhase::Escaping || Phase == EBRLevelPhase::Completed) && !bExtractionPlayed)
    {
        bExtractionPlayed=true; bRoundEnding=true;
        PlayEvent(EBRGameplaySound::KeyInsert,GetElevatorLocation(false));
        GetWorld()->GetTimerManager().SetTimer(CloseTimer,FTimerDelegate::CreateWeakLambda(this,[this]()
        {PlayEvent(EBRGameplaySound::ElevatorDoorClose,GetElevatorLocation(false));}),1.0f,false);
        GetWorld()->GetTimerManager().SetTimer(MotorTimer,FTimerDelegate::CreateWeakLambda(this,[this]()
        {PlayEvent(EBRGameplaySound::ElevatorMotor,GetElevatorLocation(false));}),3.0f,false);
    }
    if (Phase == EBRLevelPhase::Failed) bRoundEnding=true;
    if (bRoundEnding && Ambience) Ambience->AdjustVolume(1.0f,GetDefault<UBRGameplayAudioSettings>()->MasterVolume * 0.05f);
}

void UBRGameplayAudioSubsystem::Deinitialize()
{
    if (GetWorld()) GetWorld()->GetTimerManager().ClearAllTimersForObject(this);
    for (UAudioComponent* Component:Loops) if (IsValid(Component)) {Component->Stop();Component->DestroyComponent();}
    if (LogPlayback()) UE_LOG(LogTemp,Display,TEXT("BR_AUDIO result=CLEANUP loops=%d"),Loops.Num());
    Loops.Empty(); LoopCues.Empty(); Ambience=nullptr;
    Super::Deinitialize();
}

#if !UE_BUILD_SHIPPING
void UBRGameplayAudioSubsystem::StartSmoke()
{
    FApp::SetUnfocusedVolumeMultiplier(1.0f);
    FApp::SetVolumeMultiplier(1.0f);
    RecordingPath=FPaths::ProjectSavedDir()/TEXT("AudioTests");
    FParse::Value(FCommandLine::Get(),TEXT("BRAudioRecord="),RecordingPath);
    UAudioMixerBlueprintLibrary::StartRecordingOutput(this,18.0f);
    GetWorld()->GetTimerManager().SetTimer(SmokeTimer,this,&ThisClass::SmokeNext,1.0f,true,2.0f);
}
void UBRGameplayAudioSubsystem::SmokeNext()
{
    if (SmokeIndex < 12)
    {
        FVector Location=GetElevatorLocation(true);
        if (auto* Pawn=UGameplayStatics::GetPlayerPawn(this,0)) Location=Pawn->GetActorLocation();
        PlayEvent(static_cast<EBRGameplaySound>(SmokeIndex),Location);
        ++SmokeIndex; return;
    }
    if (SmokeIndex++ == 15)
    {
        UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("GameplayAudio"),RecordingPath);
        GetWorld()->GetTimerManager().ClearTimer(SmokeTimer);
        UE_LOG(LogTemp,Display,TEXT("BR_AUDIO_SMOKE result=FINISHED requested_events=12 recording=%s"),*RecordingPath);
        GetWorld()->GetTimerManager().SetTimer(SmokeQuitTimer,FTimerDelegate::CreateWeakLambda(this,[]()
        {FPlatformMisc::RequestExit(false);}),4.0f,false);
    }
}
#endif
