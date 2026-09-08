#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRStaminaComponent.h"
#include "Player/BRDownedComponent.h"
#include "AI/BREntityCharacter.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
    struct FStaminaTestWorld
    {
        UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
        ~FStaminaTestWorld() { World->DestroyWorld(false); }
        ABRPlayerCharacter* Player()
        {
            FActorSpawnParameters Params;
            Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            return World->SpawnActor<ABRPlayerCharacter>(FVector::ZeroVector, FRotator::ZeroRotator, Params);
        }
    };
    void Advance(ABRPlayerCharacter* Player, float Seconds, bool bMoving)
    {
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Player->GetCharacterMovement()->Velocity = bMoving ? FVector(600,0,0) : FVector::ZeroVector;
        Player->GetStaminaComponent()->TickComponent(Seconds, LEVELTICK_All, nullptr);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRStaminaLifecycle, "Backrooms.Stamina.DrainRecoveryAndExhaustion", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBRStaminaLifecycle::RunTest(const FString&)
{
    FStaminaTestWorld Fixture;
    auto* Player = Fixture.Player();
    auto* Stamina = Player->GetStaminaComponent();
    Player->StartSprint();
    Advance(Player, 1.f, false);
    TestEqual(TEXT("Standing with Shift held does not drain stamina"), Stamina->GetStamina(), 100.f);
    Advance(Player, 2.5f, true);
    TestEqual(TEXT("Moving sprint consumes the reserve"), Stamina->GetStamina(), 50.f);
    Player->StopSprint();
    Advance(Player, 1.f, false);
    TestEqual(TEXT("Recovery waits after sprint"), Stamina->GetStamina(), 50.f);
    Advance(Player, 1.f, false);
    TestEqual(TEXT("Only time after the delay recovers stamina"), Stamina->GetStamina(), 62.5f);
    Player->StartSprint();
    Advance(Player, 10.f, true);
    TestEqual(TEXT("Exhaustion clamps at zero"), Stamina->GetStamina(), 0.f);
    TestFalse(TEXT("Exhaustion stops sprinting"), Player->IsSprinting());
    TestEqual(TEXT("Exhaustion restores walking speed"), Player->GetCharacterMovement()->MaxWalkSpeed, 350.f);
    for (int32 I=0; I<10; ++I) Player->StartSprint();
    TestFalse(TEXT("Repeated sprint input cannot bypass exhaustion"), Player->IsSprinting());
    Advance(Player, 1.9f, false);
    TestFalse(TEXT("Small recovery does not cause sprint/walk oscillation"), Stamina->CanSprint());
    Advance(Player, 0.5f, false);
    TestTrue(TEXT("Sprinting becomes available after sufficient recovery"), Stamina->CanSprint());
    Player->StopSprint();
    Advance(Player, 20.f, false);
    TestEqual(TEXT("Recovery cannot overfill"), Stamina->GetStamina(), 100.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRStaminaChasers, "Backrooms.Stamina.MultipleEntitiesAndTargetChanges", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBRStaminaChasers::RunTest(const FString&)
{
    FStaminaTestWorld Fixture;
    auto* Player = Fixture.Player();
    auto* Other = Fixture.Player();
    auto* Stamina = Player->GetStaminaComponent();
    Player->StartSprint();
    Advance(Player, 5.f, true);
    auto* Entity = Fixture.World->SpawnActor<ABREntityCharacter>();
    auto* OtherEntity = Fixture.World->SpawnActor<AActor>();
    Entity->SetEntityState(EBREntityState::Investigating, Player);
    TestFalse(TEXT("Investigation alone is not a chase"), Stamina->HasUnlimitedStamina());
    for (int32 I=0; I<10; ++I) Entity->SetEntityState(EBREntityState::Chasing, Player);
    TestTrue(TEXT("A chase immediately permits sprint at zero"), Player->IsSprinting());
    Advance(Player, 60.f, true);
    TestEqual(TEXT("Chased sprint does not consume or refill the stored reserve"), Stamina->GetStamina(), 0.f);
    TestFalse(TEXT("Unrelated player is not granted unlimited stamina"), Other->GetStaminaComponent()->HasUnlimitedStamina());
    Stamina->SetChasedBy(OtherEntity, true);
    Entity->SetEntityState(EBREntityState::Chasing, Other);
    TestTrue(TEXT("Another type of entity keeps the first chase active"), Stamina->HasUnlimitedStamina());
    TestTrue(TEXT("Changed target receives its own chase status"), Other->GetStaminaComponent()->HasUnlimitedStamina());
    Entity->SetEntityState(EBREntityState::Returning);
    TestFalse(TEXT("Losing the target clears the other player's chase"), Other->GetStaminaComponent()->HasUnlimitedStamina());
    OtherEntity->Destroy();
    Advance(Player, 0.05f, true);
    TestFalse(TEXT("Destroyed last chaser releases unlimited stamina"), Stamina->HasUnlimitedStamina());
    TestFalse(TEXT("Normal exhaustion applies again after the chase"), Player->IsSprinting());
    Entity->SetEntityState(EBREntityState::Chasing, Player);
    Player->GetDownedComponent()->Down();
    Player->StartSprint();
    TestFalse(TEXT("Unlimited stamina cannot override being downed"), Player->IsSprinting());
    return true;
}
#endif
