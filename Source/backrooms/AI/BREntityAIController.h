#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BREntityAIController.generated.h"

class ABREntityCharacter;
class ABRPlayerCharacter;
class UAIPerceptionComponent;
class UAISenseConfig_Hearing;
class UAISenseConfig_Sight;

UCLASS()
class BACKROOMS_API ABREntityAIController : public AAIController
{
    GENERATED_BODY()
public:
    ABREntityAIController();

protected:
    virtual void OnPossess(APawn* InPawn) override;
    virtual void OnUnPossess() override;
    virtual void OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result) override;

    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="cm")) float PatrolRadius = 25000.f;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="cm")) float MinPatrolDistance = 1400.f;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="cm/s")) float PatrolSpeed = 150.f;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="cm/s")) float ChaseSpeed = 575.f;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="s")) float SightMemorySeconds = 7.f;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="s")) float StuckSeconds = 3.f;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|AI", meta=(Units="cm")) float RetreatDistance = 1600.f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Backrooms|AI") TObjectPtr<UAIPerceptionComponent> EntityPerception;
    UPROPERTY() TObjectPtr<UAISenseConfig_Sight> SightConfig;
    UPROPERTY() TObjectPtr<UAISenseConfig_Hearing> HearingConfig;
    UFUNCTION() void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

private:
    void UpdateBehavior();
    bool IsEligibleTarget(const ABRPlayerCharacter* Player) const;
    bool CanCurrentlySee(ABRPlayerCharacter* Player) const;
    void StartChase(ABRPlayerCharacter* Player);
    void BeginReturn(bool bUnreachable);
    bool ChoosePatrolGoal(FVector& OutLocation);
    void RememberLocation(const FVector& Location);
    void RememberBlockedLocation();
    void TraceBehavior(const TCHAR* Event) const;
    FTimerHandle BehaviorTimer;
    TWeakObjectPtr<ABRPlayerCharacter> UnreachableTarget;
    double UnreachableTargetUntil = 0;
    double LastSeenTime = 0;
    double InvestigationEnds = 0;
    double AttackCooldown = 0;
    double NextMoveAttempt = 0;
    double LastProgressTime = 0;
    double AvoidLocationUntil = 0;
    FVector LastSeenLocation = FVector::ZeroVector;
    FVector LastProgressLocation = FVector::ZeroVector;
    FVector ReturnStart = FVector::ZeroVector;
    FVector AvoidLocation = FVector::ZeroVector;
    TArray<FVector> RecentLocations;
    bool bMoveFailed = false;
    bool bTraceBehavior = false;
};
