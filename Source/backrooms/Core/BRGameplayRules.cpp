#include "Core/BRGameplayRules.h"

int32 FBRGameplayRules::ClampCompletedObjectives(const int32 CompletedObjectives, const int32 TotalObjectives)
{
	return FMath::Clamp(CompletedObjectives, 0, FMath::Max(0, TotalObjectives));
}

bool FBRGameplayRules::AreObjectivesComplete(const int32 CompletedObjectives, const int32 TotalObjectives)
{
	return TotalObjectives > 0 && CompletedObjectives >= TotalObjectives;
}

bool FBRGameplayRules::CanExtract(const int32 ActivePlayers, const int32 PlayersInZone, const bool bObjectivesComplete)
{
	return bObjectivesComplete && ActivePlayers > 0 && PlayersInZone >= ActivePlayers;
}
