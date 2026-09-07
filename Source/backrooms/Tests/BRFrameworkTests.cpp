#if WITH_DEV_AUTOMATION_TESTS

#include "Core/BRGameplayRules.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FBRGameplayRulesTest,
	"Backrooms.Framework.GameplayRules",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBRGameplayRulesTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Completed objectives clamp below zero"), FBRGameplayRules::ClampCompletedObjectives(-2, 3), 0);
	TestEqual(TEXT("Completed objectives clamp to total"), FBRGameplayRules::ClampCompletedObjectives(5, 3), 3);
	TestFalse(TEXT("Zero registered objectives are not complete"), FBRGameplayRules::AreObjectivesComplete(0, 0));
	TestTrue(TEXT("Registered objectives complete at total"), FBRGameplayRules::AreObjectivesComplete(3, 3));
	TestFalse(TEXT("No extraction while objectives are incomplete"), FBRGameplayRules::CanExtract(4, 4, false));
	TestFalse(TEXT("No extraction while one active player is missing"), FBRGameplayRules::CanExtract(4, 3, true));
	TestTrue(TEXT("Extraction succeeds when every active player is present"), FBRGameplayRules::CanExtract(4, 4, true));
	return true;
}

#endif
