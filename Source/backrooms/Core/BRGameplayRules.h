#pragma once

#include "CoreMinimal.h"

struct BACKROOMS_API FBRGameplayRules
{
	static int32 ClampCompletedObjectives(int32 CompletedObjectives, int32 TotalObjectives);
	static bool AreObjectivesComplete(int32 CompletedObjectives, int32 TotalObjectives);
	static bool CanExtract(int32 ActivePlayers, int32 PlayersInZone, bool bObjectivesComplete);
};
