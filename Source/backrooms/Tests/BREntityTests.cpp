#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "AI/BREntityCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/UnrealType.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBREntityFacingContracts, "Backrooms.Entity.FacingAndRoarContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBREntityFacingContracts::RunTest(const FString&)
{
    const auto* Entity = GetDefault<ABREntityCharacter>();
    TestFalse(TEXT("Controller yaw does not override movement"), Entity->bUseControllerRotationYaw);
    TestTrue(TEXT("Turn toward movement"), bool(Entity->GetCharacterMovement()->bOrientRotationToMovement));
    TestFalse(TEXT("No second competing yaw driver"), bool(Entity->GetCharacterMovement()->bUseControllerDesiredRotation));
    TestTrue(TEXT("Mesh forward +Y becomes actor +X"), FVector::DotProduct(Entity->GetMesh()->GetRelativeRotation().RotateVector(FVector::YAxisVector), FVector::XAxisVector)>0.999f);
    const auto* Roar = Entity->FindFunction(TEXT("MulticastPlayRoar"));
    TestTrue(TEXT("Roar is server multicast, not a local perception sound"), Roar && Roar->HasAllFunctionFlags(FUNC_Net | FUNC_NetMulticast));
    return true;
}
#endif
