#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "BRGameplayAudioSettings.generated.h"

class USoundWave;

UENUM(BlueprintType)
enum class EBRGameplaySound : uint8
{
    Ambience, DoorOpen, KeyPickup, KeyInsert,
    ElevatorDoorOpen, ElevatorDoorClose, ElevatorBell, ElevatorMotor,
    SkinStealerChase, SkinStealerAttack, SkinStealerStep1, SkinStealerStep2,
    SkinStealerRoar, SkinStealerImpact, SkinStealerPain, Count UMETA(Hidden)
};

USTRUCT(BlueprintType)
struct FBRGameplaySoundEntry
{
    GENERATED_BODY()
    UPROPERTY(EditAnywhere, Category="Sound") TSoftObjectPtr<USoundWave> Sound;
    /** Optional alternatives; the first Sound remains variation zero. */
    UPROPERTY(EditAnywhere, Category="Sound") TArray<TSoftObjectPtr<USoundWave>> Variations;
    UPROPERTY(EditAnywhere, Category="Sound", meta=(ClampMin="0", ClampMax="2")) float Volume = 0.6f;
    UPROPERTY(EditAnywhere, Category="Sound") bool bSpatial = true;
    UPROPERTY(EditAnywhere, Category="Sound") bool bOcclusion = true;
    UPROPERTY(EditAnywhere, Category="Sound", meta=(ClampMin="0", Units="cm")) float InnerRadius = 100.0f;
    UPROPERTY(EditAnywhere, Category="Sound", meta=(ClampMin="1", Units="cm")) float FalloffDistance = 1200.0f;
};

/** Project Settings > Game > Parking Garage Audio. Changes apply on the next play session. */
UCLASS(Config=Game, DefaultConfig, meta=(DisplayName="Parking Garage Audio"))
class BACKROOMS_API UBRGameplayAudioSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UBRGameplayAudioSettings();
    virtual FName GetCategoryName() const override { return TEXT("Game"); }
    UPROPERTY(EditAnywhere, Config, Category="Mix") bool bEnabled = true;
    UPROPERTY(EditAnywhere, Config, Category="Mix", meta=(ClampMin="0", ClampMax="1")) float MasterVolume = 0.8f;
    UPROPERTY(EditAnywhere, Config, Category="Mix", meta=(ClampMin="0", ClampMax="1")) float OccludedVolume = 0.25f;
    UPROPERTY(EditAnywhere, Config, Category="Mix", meta=(ClampMin="100", ClampMax="20000", Units="Hz")) float OccludedLowPassHz = 1800.0f;
    UPROPERTY(EditAnywhere, Config, Category="Sounds") TMap<EBRGameplaySound, FBRGameplaySoundEntry> Sounds;
    UPROPERTY(EditAnywhere, Config, Category="SkinStealer", meta=(ClampMin="50", Units="cm")) float WalkStepDistance = 115.0f;
    UPROPERTY(EditAnywhere, Config, Category="SkinStealer", meta=(ClampMin="50", Units="cm")) float RunStepDistance = 160.0f;
    UPROPERTY(EditAnywhere, Config, Category="SkinStealer", meta=(ClampMin="2", Units="s")) float RoarInterval = 4.0f;
    UPROPERTY(EditAnywhere, Config, Category="Debug") bool bLogPlayback = false;
    static bool IsLoop(EBRGameplaySound Event);
};
