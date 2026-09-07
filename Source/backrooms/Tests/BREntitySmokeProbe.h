#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BREntitySmokeProbe.generated.h"
class ABREntityCharacter;
class ABRPlayerCharacter;
class AAIController;

/** Explicit -BREntitySmoke development fixture; never saved into a map. */
UCLASS()
class ABREntitySmokeProbe : public AActor
{
    GENERATED_BODY()
public:
    ABREntitySmokeProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    void Check(bool bOK, const TCHAR* Name);
    void Finish();
    UPROPERTY(Replicated) int32 Stage = -1;
    UPROPERTY(Replicated) TObjectPtr<ABREntityCharacter> Entity;
    UPROPERTY() TObjectPtr<ABRPlayerCharacter> Player;
    UPROPERTY() TObjectPtr<AAIController> OriginalController;
    UPROPERTY() TObjectPtr<AAIController> MovementController;
    double Started = 0, StageStarted = 0;
    int32 Failures = 0, Checks = 0, InitialRoars = 0, LastRoars = 0;
    bool bSampled = false;
    uint8 ClientDirections = 0;
};
