#include "Core/BRGameState.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Engine/World.h"

#include "Core/BRGameplayRules.h"
#include "Net/UnrealNetwork.h"

ABRGameState::ABRGameState()
{
	SetNetUpdateFrequency(10.0f);
}

void ABRGameState::RegisterObjective()
{
	if (HasAuthority())
	{
		++TotalObjectives;
		OnRep_Objectives();
		ForceNetUpdate();
	}
}

void ABRGameState::NotifyObjectiveCompleted()
{
	if (HasAuthority())
	{
		CompletedObjectives = FBRGameplayRules::ClampCompletedObjectives(CompletedObjectives + 1, TotalObjectives);
		if (AreObjectivesComplete())
		{
			SetLevelPhase(EBRLevelPhase::ExtractionReady);
		}
		OnRep_Objectives();
		ForceNetUpdate();
	}
}

bool ABRGameState::AreObjectivesComplete() const
{
	return FBRGameplayRules::AreObjectivesComplete(CompletedObjectives, TotalObjectives);
}

void ABRGameState::SetLevelPhase(const EBRLevelPhase NewPhase)
{
	if (HasAuthority() && LevelPhase != NewPhase)
	{
		LevelPhase = NewPhase;
		OnRep_LevelPhase();
		ForceNetUpdate();
	}
}

void ABRGameState::OnRep_Objectives()
{
	OnObjectivesChanged.Broadcast(CompletedObjectives, TotalObjectives);
}

void ABRGameState::OnRep_LevelPhase()
{
	OnLevelPhaseChanged.Broadcast(LevelPhase);
    if (auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>()) Audio->HandleLevelPhase(LevelPhase);
}

void ABRGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABRGameState, RoomInfo);
	DOREPLIFETIME(ABRGameState, TotalObjectives);
	DOREPLIFETIME(ABRGameState, CompletedObjectives);
	DOREPLIFETIME(ABRGameState, LevelPhase);
}

void ABRGameState::SetRoomInfo(const FBRRoomInfo& Info)
{
 if (HasAuthority()) { RoomInfo = Info; ForceNetUpdate(); }
}
