#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/BRFrameworkTypes.h"
#include "Online/BRRoomTypes.h"
#include "BRGameState.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRObjectivesChanged, int32, CompletedObjectives, int32, TotalObjectives);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRLevelPhaseChanged, EBRLevelPhase, NewPhase);

UCLASS()
class BACKROOMS_API ABRGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	ABRGameState();
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Backrooms|Room") FBRRoomInfo RoomInfo;
	void SetRoomInfo(const FBRRoomInfo& Info);

	void RegisterObjective();
	void NotifyObjectiveCompleted();

	UFUNCTION(BlueprintPure, Category = "Backrooms|Objectives")
	bool AreObjectivesComplete() const;

	UFUNCTION(BlueprintPure, Category = "Backrooms|Objectives")
	int32 GetCompletedObjectives() const { return CompletedObjectives; }

	UFUNCTION(BlueprintPure, Category = "Backrooms|Objectives")
	int32 GetTotalObjectives() const { return TotalObjectives; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Level")
	void SetLevelPhase(EBRLevelPhase NewPhase);

	UFUNCTION(BlueprintPure, Category = "Backrooms|Level")
	EBRLevelPhase GetLevelPhase() const { return LevelPhase; }

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Objectives")
	FBRObjectivesChanged OnObjectivesChanged;

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Level")
	FBRLevelPhaseChanged OnLevelPhaseChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Objectives, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Objectives")
	int32 TotalObjectives = 0;

	UPROPERTY(ReplicatedUsing = OnRep_Objectives, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Objectives")
	int32 CompletedObjectives = 0;

	UPROPERTY(ReplicatedUsing = OnRep_LevelPhase, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Level")
	EBRLevelPhase LevelPhase = EBRLevelPhase::Lobby;

	UFUNCTION()
	void OnRep_Objectives();

	UFUNCTION()
	void OnRep_LevelPhase();
};
