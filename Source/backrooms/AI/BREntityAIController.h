#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BREntityAIController.generated.h"

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
    void UpdateBehavior();
    FTimerHandle BehaviorTimer;
    float AttackCooldown = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|AI")
	TObjectPtr<UAIPerceptionComponent> EntityPerception;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	UPROPERTY()
	TObjectPtr<UAISenseConfig_Hearing> HearingConfig;

	UFUNCTION()
	void OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);
};
