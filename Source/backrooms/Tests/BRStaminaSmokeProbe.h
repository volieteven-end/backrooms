#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRStaminaSmokeProbe.generated.h"
class ABRPlayerCharacter;
class ABREntityCharacter;

/** Explicit development fixture; runs real client input and server stamina over time. */
UCLASS(Transient, NotPlaceable)
class ABRStaminaSmokeProbe : public AActor
{
    GENERATED_BODY()
public:
    ABRStaminaSmokeProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
private:
    void Check(bool bOK, const TCHAR* Name);
    void SetStage(int32 Next);
    UPROPERTY(Replicated) int32 Stage = 0;
    UPROPERTY(Replicated) TObjectPtr<ABRPlayerCharacter> Player;
    UPROPERTY() TObjectPtr<ABRPlayerCharacter> Other;
    UPROPERTY() TObjectPtr<ABREntityCharacter> Entity;
    UPROPERTY() TObjectPtr<AActor> AdditionalEntity;
    double Started = 0, StageStarted = 0;
    int32 Failures = 0, Checks = 0, ClientInputStage = -1;
    TSet<FName> Observed;
};
