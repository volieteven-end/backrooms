#pragma once

#include "CoreMinimal.h"
#include "BRFrameworkTypes.generated.h"

UENUM(BlueprintType)
enum class EBRLevelPhase : uint8
{
	Lobby,
	Exploring,
	ExtractionReady,
	Escaping,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class EBREntityState : uint8
{
	Idle,
	Patrolling,
	Investigating,
	Chasing,
	Returning
};
