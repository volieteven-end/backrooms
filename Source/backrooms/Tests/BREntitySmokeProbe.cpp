#include "Tests/BREntitySmokeProbe.h"
#include "AI/BREntityCharacter.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "Core/BRGameState.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"

ABREntitySmokeProbe::ABREntitySmokeProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    // CharacterMovement consumes requested velocity every frame, including NullRHI runs.
    PrimaryActorTick.TickInterval = 0;
    bReplicates = true;
    bAlwaysRelevant = true;
}
void ABREntitySmokeProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABREntitySmokeProbe, Stage);
    DOREPLIFETIME(ABREntitySmokeProbe, Entity);
}
void ABREntitySmokeProbe::Check(bool bOK, const TCHAR* Name)
{
    ++Checks;
    if (!bOK) ++Failures;
    UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_TEST case=%s result=%s net=%d"), Name, bOK ? TEXT("PASS") : TEXT("FAIL"), int32(GetNetMode()));
}
void ABREntitySmokeProbe::Finish()
{
    UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_TEST result=%s checks=%d failures=%d"), Failures ? TEXT("FAIL") : TEXT("PASS"), Checks, Failures);
    Stage = 99;
    if (GetNetMode() == NM_DedicatedServer)
        GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->FinishRound(Failures == 0);
    else FPlatformMisc::RequestExitWithStatus(false, Failures ? 2 : 0);
}
void ABREntitySmokeProbe::Tick(float DT)
{
    Super::Tick(DT);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority())
    {
        if (Entity && Stage >= 0 && Stage < 4 && !(ClientDirections & (1 << Stage)) && Entity->GetVelocity().Size2D() > 50)
        {
            const float Dot = FVector::DotProduct(Entity->GetVelocity().GetSafeNormal2D(), Entity->GetMesh()->GetRightVector().GetSafeNormal2D());
            if (Dot > 0.97f)
            {
                ClientDirections |= 1 << Stage;
                UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_CLIENT direction=%d facing_dot=%.3f result=PASS"), Stage, Dot);
            }
        }
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Started) Started = Now;
    if (Stage == 99) return;
    if (Now - Started > 60) { Check(false, TEXT("TIMEOUT")); Finish(); return; }
    if (Stage == -1)
    {
        if (Now - Started < 2) return;
        for (TActorIterator<ABREntityCharacter> It(GetWorld()); It; ++It) { Entity = *It; break; }
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld()); It; ++It) if (It->IsPlayerControlled()) { Player = *It; break; }
        if (!Entity || !Player) return;
        OriginalController = Cast<AAIController>(Entity->GetController());
        if (!OriginalController) { Check(false, TEXT("AI_CONTROLLER")); Finish(); return; }
        OriginalController->UnPossess();
        MovementController = GetWorld()->SpawnActor<AAIController>();
        MovementController->Possess(Entity);
        auto* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-20000, -20000, 3950), FRotator::ZeroRotator);
        Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
        Floor->SetActorScale3D(FVector(160, 160, 1));
        Entity->TeleportTo(FVector(-20000, -20000, 4000 + Entity->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3), FRotator::ZeroRotator, false, true);
        Entity->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Entity->GetCharacterMovement()->MaxWalkSpeed = 200;
        Entity->SetEntityState(EBREntityState::Patrolling);
        int32 I = 0;
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld()); It; ++It, ++I)
        {
            It->TeleportTo(FVector(-19000, -20000 + I * 200, 4000 + It->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + 3), FRotator(0,180,0), false, true);
            It->GetCharacterMovement()->DisableMovement();
            if (*It != Player) It->Tags.AddUnique(TEXT("BR_Hiding"));
        }
        Check(!Entity->bUseControllerRotationYaw && Entity->GetCharacterMovement()->bOrientRotationToMovement, TEXT("FACING_FLAGS"));
        Check(FVector::DotProduct(Entity->GetActorForwardVector(), Entity->GetMesh()->GetRightVector()) > 0.999f, TEXT("MESH_PLUS_Y_TO_ACTOR_PLUS_X"));
        InitialRoars = Entity->GetRoarCueCount();
        Stage = 0; StageStarted = Now;
    }
    if (Stage >= 0 && Stage < 4)
    {
        const FVector Directions[] = {FVector(1,0,0), FVector(0,1,0), FVector(-1,0,0), FVector(0,-1,0)};
        // Exercise the same requested-velocity path used by AI path following.
        Entity->GetCharacterMovement()->RequestDirectMove(Directions[Stage] * 200.f, true);
        if (!bSampled && Now - StageStarted > 1.5)
        {
            const float Dot = FVector::DotProduct(Entity->GetVelocity().GetSafeNormal2D(), Entity->GetMesh()->GetRightVector().GetSafeNormal2D());
            UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_FACING direction=%d dot=%.4f speed=%.1f mode=%d active=%d location=%s"), Stage, Dot, Entity->GetVelocity().Size2D(), int32(Entity->GetCharacterMovement()->MovementMode), Entity->GetCharacterMovement()->IsActive(), *Entity->GetActorLocation().ToCompactString());
            Check(Dot > 0.98f && Entity->GetVelocity().Size2D() > 100, *FString::Printf(TEXT("MOVE_DIRECTION_%d"), Stage));
            bSampled = true;
        }
        if (Now - StageStarted > 2.5)
        {
            ++Stage; StageStarted = Now; bSampled = false;
            if (Stage == 4)
            {
                Check(Entity->GetRoarCueCount() == InitialRoars, TEXT("PATROL_SILENT"));
                MovementController->UnPossess();
                Entity->GetCharacterMovement()->DisableMovement();
                Entity->SetActorRotation(FRotator::ZeroRotator);
                Player->TeleportTo(Entity->GetActorLocation() + FVector(1000,0,0), FRotator(0,180,0), false, true);
                OriginalController->Possess(Entity);
                OriginalController->SetControlRotation(FRotator::ZeroRotator);
                OriginalController->GetPerceptionComponent()->ForgetAll();
                OriginalController->GetPerceptionComponent()->RequestStimuliListenerUpdate();
            }
        }
        return;
    }
    if (Stage == 4 && Now - StageStarted > 2.5)
    {
        Check(Entity->GetEntityState() == EBREntityState::Chasing && Entity->GetTargetActor() == Player, TEXT("REAL_SIGHT_DETECTS_PLAYER"));
        Check(Entity->GetRoarCueCount() == InitialRoars + 1, TEXT("DISCOVERY_ONE_ROAR"));
        Stage = 5; // Keep timing from initial sight setup for a full repeat window.
    }
    if (Stage == 5 && Now - StageStarted > 9.5)
    {
        Check(Entity->GetRoarCueCount() == InitialRoars + 3, TEXT("CHASE_ROARS_EVERY_FOUR_SECONDS"));
        LastRoars = Entity->GetRoarCueCount();
        for (int32 I=0; I<20; ++I) Entity->SetEntityState(EBREntityState::Chasing, Player);
        Check(Entity->GetRoarCueCount() == LastRoars, TEXT("NO_PERCEPTION_SPAM"));
        OriginalController->UnPossess();
        Entity->SetEntityState(EBREntityState::Returning);
        Stage = 6; StageStarted = Now;
    }
    if (Stage == 6 && Now - StageStarted > 4.5)
    {
        Check(Entity->GetRoarCueCount() == LastRoars, TEXT("LOST_TARGET_STOPS_ROARS"));
        auto* GS = GetWorld()->GetGameState<ABRGameState>();
        GS->SetLevelPhase(EBRLevelPhase::Completed);
        Entity->SetEntityState(EBREntityState::Chasing, Player);
        Check(Entity->GetRoarCueCount() == LastRoars, TEXT("ROUND_END_SILENT"));
        GS->SetLevelPhase(EBRLevelPhase::Exploring);
        Player->GetDownedComponent()->Down();
        Entity->SetEntityState(EBREntityState::Chasing, Player);
        Check(Entity->GetRoarCueCount() == LastRoars, TEXT("DOWNED_TARGET_SILENT"));
        Entity->SetEntityState(EBREntityState::Returning);
        Finish();
    }
#endif
}
