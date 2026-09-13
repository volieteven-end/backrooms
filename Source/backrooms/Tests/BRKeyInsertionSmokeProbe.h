#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRKeyInsertionSmokeProbe.generated.h"
class ABRPlayerCharacter;
class ABRGarageKeyManager;
class ABRExtractionZone;

/** Explicit development fixture: real E input, replicated sockets, physical exit and UMG result. */
UCLASS(Transient, NotPlaceable)
class ABRKeyInsertionSmokeProbe : public AActor
{
    GENERATED_BODY()
public:
    ABRKeyInsertionSmokeProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    void Check(bool OK,const TCHAR* Name);
    void SetStage(int32 Value);
    void PlaceAtSocket(int32 Index,bool Both=false);
    void PlaceAtSpawnKey(int32 Index);
    void CollectKeys(int32 Count);
    void TickLocal();
    void Capture(const TCHAR* Name);
    UPROPERTY(Replicated) int32 Stage=0;
    UPROPERTY(Replicated) TObjectPtr<ABRPlayerCharacter> Subject;
    UPROPERTY(Replicated) TObjectPtr<ABRGarageKeyManager> Manager;
    UPROPERTY() TObjectPtr<ABRPlayerCharacter> Other;
    UPROPERTY() TObjectPtr<ABRExtractionZone> Zone;
    double Started=0,StageStarted=0,LocalStageStarted=0;
    int32 LocalStage=-1,Checks=0,Failures=0;
    bool bPressed=false,bResultObserved=false;
    TSet<FName> Observed;
};
