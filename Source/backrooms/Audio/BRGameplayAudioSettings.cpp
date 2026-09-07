#include "Audio/BRGameplayAudioSettings.h"
#include "Sound/SoundWave.h"

UBRGameplayAudioSettings::UBRGameplayAudioSettings()
{
    auto Add = [this](EBRGameplaySound Event, const TCHAR* Relative, float Volume, bool Spatial, float Range)
    {
        FBRGameplaySoundEntry Entry;
        const FString Package = FString(TEXT("/Game/ReverseAsset/ParkingGarage/Audio/")) + Relative;
        FString Name; Package.Split(TEXT("/"), nullptr, &Name, ESearchCase::CaseSensitive, ESearchDir::FromEnd);
        Entry.Sound = TSoftObjectPtr<USoundWave>(FSoftObjectPath(Package + TEXT(".") + Name));
        Entry.Volume = Volume; Entry.bSpatial = Spatial; Entry.bOcclusion = Spatial; Entry.FalloffDistance = Range;
        Sounds.Add(Event, Entry);
    };
    Add(EBRGameplaySound::Ambience,TEXT("Ambience/Level1_Ambience3"),0.20f,false,1);
    Add(EBRGameplaySound::DoorOpen,TEXT("Interaction/Simple_Old_Door_Complete_v1_wav"),0.65f,true,1400);
    Add(EBRGameplaySound::KeyPickup,TEXT("Interaction/TakeKey"),0.75f,true,550);
    Add(EBRGameplaySound::KeyInsert,TEXT("Interaction/Keys_Inserting_v4_wav"),0.65f,true,900);
    Add(EBRGameplaySound::ElevatorDoorOpen,TEXT("Elevator/door_open"),0.65f,true,1500);
    Add(EBRGameplaySound::ElevatorDoorClose,TEXT("Elevator/door_close"),0.65f,true,1500);
    Add(EBRGameplaySound::ElevatorBell,TEXT("Elevator/Elevator_Bell"),0.55f,true,2000);
    Add(EBRGameplaySound::ElevatorMotor,TEXT("Elevator/Elevator_Loop"),0.35f,true,1500);
    Add(EBRGameplaySound::SkinStealerChase,TEXT("SkinStealer/skinstealer_chaseloop"),0.38f,true,1800);
    Add(EBRGameplaySound::SkinStealerAttack,TEXT("SkinStealer/skinstealer_gotcha1__1_"),0.65f,true,1100);
    Add(EBRGameplaySound::SkinStealerStep1,TEXT("SkinStealer/skinstealer_schlep1"),0.45f,true,1200);
    Add(EBRGameplaySound::SkinStealerStep2,TEXT("SkinStealer/skinstealer_schlep2"),0.45f,true,1200);
}

bool UBRGameplayAudioSettings::IsLoop(EBRGameplaySound Event)
{
    return Event == EBRGameplaySound::Ambience || Event == EBRGameplaySound::ElevatorMotor || Event == EBRGameplaySound::SkinStealerChase;
}
