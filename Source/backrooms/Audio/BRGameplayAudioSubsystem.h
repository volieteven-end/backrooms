#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Audio/BRGameplayAudioSettings.h"
#include "Core/BRFrameworkTypes.h"
#include "BRGameplayAudioSubsystem.generated.h"

class UAudioComponent;
class USoundCue;

/** Local presentation only. Gameplay decisions still come from replicated server state. */
UCLASS()
class BACKROOMS_API UBRGameplayAudioSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
    virtual void OnWorldBeginPlay(UWorld& World) override;
    virtual void Deinitialize() override;
    UFUNCTION(BlueprintCallable, Category="Backrooms|Audio")
    UAudioComponent* PlayEvent(EBRGameplaySound Event, FVector Location, AActor* AttachTo = nullptr, float Pitch = 1.0f);
    void HandleLevelPhase(EBRLevelPhase Phase);
    bool IsRoundEnding() const { return bRoundEnding; }
    static USoundCue* BuildLoopCue(UObject* Outer, USoundWave* Wave);

private:
    UPROPERTY(Transient) TMap<EBRGameplaySound,TObjectPtr<USoundCue>> LoopCues;
    UPROPERTY(Transient) TArray<TObjectPtr<UAudioComponent>> Loops;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> Ambience;
    FTimerHandle IntroTimer, CloseTimer, MotorTimer;
    bool bGameplayWorld = false;
    bool bRoundEnding = false;
    bool bExtractionPlayed = false;
    bool bReadyBellPlayed = false;
    FVector GetElevatorLocation(bool bArrival) const;
    bool LogPlayback() const;
#if !UE_BUILD_SHIPPING
    void StartSmoke();
    void SmokeNext();
    FTimerHandle SmokeTimer, SmokeQuitTimer;
    int32 SmokeIndex = 0;
    FString RecordingPath;
#endif
};
