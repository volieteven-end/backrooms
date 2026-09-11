#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BREntityAISmokeProbe.generated.h"

class ABREntityCharacter;
class ABREntityAIController;
class ABRPlayerCharacter;
class ABRGarageDoor;

/** Opt-in, in-memory fixtures on the real garage map; never spawned in Shipping. */
UCLASS()
class ABREntityAISmokeProbe : public AActor
{
    GENERATED_BODY()
public:
    ABREntityAISmokeProbe();
    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
private:
    void Check(bool OK, const TCHAR* Name);
    void SetStage(int32 Value);
    void Finish();
    void TickLocal();
    bool HasFullPath(FVector From, FVector To) const;
    bool DoorPoints(ABRGarageDoor* Door, FVector& A, FVector& B) const;
    void PlacePawn(APawn* Pawn, FVector Ground, FVector Forward);
    void PrepareDoorChase();
    UPROPERTY(Replicated) int32 Stage = -1;
    UPROPERTY(Replicated) TObjectPtr<ABREntityCharacter> Entity;
    UPROPERTY(Replicated) TObjectPtr<ABRPlayerCharacter> Subject;
    UPROPERTY() TObjectPtr<ABREntityAIController> Controller;
    UPROPERTY() TArray<TObjectPtr<ABRGarageDoor>> Doors;
    UPROPERTY() TObjectPtr<ABRGarageDoor> ChaseDoor;
    double Started = 0, StageStarted = 0, NextSample = 0;
    int32 Checks = 0, Failures = 0, DoorIndex = 0, GoalCount = 0;
    int32 LocalStage = -10, LocalAttackStart = 0, ServerAttackStart = 0;
    double LocalStageStarted = 0;
    float Travel = 0, MaxPatrolDistance = 0, MinGoalDistance = MAX_flt;
    FVector PatrolOrigin, LastSample, LastGoal, Outside, Inside, DoorCenter, TowardDoor, ReturnOrigin, DoorA, DoorB;
    TSet<FIntPoint> PatrolCells;
    bool bCheckedGrace = false, bRecorded = false, bClientAttackChecked = false;
    bool bDoorScreenshot = false, bAttackScreenshot = false;
};
