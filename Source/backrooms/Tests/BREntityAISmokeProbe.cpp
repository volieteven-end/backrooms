#include "Tests/BREntityAISmokeProbe.h"
#include "AI/BREntityAIController.h"
#include "AI/BREntityCharacter.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "Player/BRStaminaComponent.h"
#include "World/BRGarageDoor.h"
#include "World/BRGarageExitDoor.h"
#include "World/BRLootCabinet.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Core/BRGameState.h"
#include "AudioMixerBlueprintLibrary.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "HighResScreenshot.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "Net/UnrealNetwork.h"

ABREntityAISmokeProbe::ABREntityAISmokeProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = bAlwaysRelevant = true;
}
void ABREntityAISmokeProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABREntityAISmokeProbe, Stage);
    DOREPLIFETIME(ABREntityAISmokeProbe, Entity);
    DOREPLIFETIME(ABREntityAISmokeProbe, Subject);
}
void ABREntityAISmokeProbe::Check(bool OK, const TCHAR* Name)
{
    ++Checks; if (!OK) ++Failures;
    UE_LOG(LogTemp, Display, TEXT("BR_AI_TEST case=%s result=%s stage=%d"), Name, OK ? TEXT("PASS") : TEXT("FAIL"), Stage);
}
void ABREntityAISmokeProbe::SetStage(int32 Value)
{
    Stage = Value; StageStarted = GetWorld()->GetTimeSeconds(); ForceNetUpdate();
    UE_LOG(LogTemp, Display, TEXT("BR_AI_TEST stage=%d time=%.2f"), Stage, StageStarted);
}
void ABREntityAISmokeProbe::Finish()
{
    UE_LOG(LogTemp, Display, TEXT("BR_AI_TEST result=%s checks=%d failures=%d"), Failures ? TEXT("FAIL") : TEXT("PASS"), Checks, Failures);
    SetStage(99);
    if (GetNetMode() == NM_DedicatedServer) GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->FinishRound(Failures == 0);
    else FPlatformMisc::RequestExitWithStatus(false, Failures ? 2 : 0);
}
bool ABREntityAISmokeProbe::HasFullPath(FVector From, FVector To) const
{
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    auto* Data = Nav && Entity ? Nav->GetNavDataForProps(Entity->GetNavAgentPropertiesRef(), From) : nullptr;
    if (!Data) return false;
    FPathFindingQuery Query(Controller, *Data, From, To);
    Query.SetAllowPartialPaths(false);
    const FPathFindingResult Result = Nav->FindPathSync(Entity->GetNavAgentPropertiesRef(), Query);
    return Result.IsSuccessful() && Result.Path.IsValid() && !Result.Path->IsPartial() && FVector::Dist2D(Result.Path->GetEndLocation(), To) < 80;
}
bool ABREntityAISmokeProbe::DoorPoints(ABRGarageDoor* Door, FVector& A, FVector& B) const
{
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    for (TActorIterator<AActor> It(GetWorld()); It; ++It)
        if (Door->OwnsPanel(*It))
            if (const auto* Mesh = It->FindComponentByClass<UStaticMeshComponent>())
            {
                FNavLocation PA, PB;
                const FVector Center = Mesh->Bounds.Origin;
                const FVector Delta = Door->GetActorForwardVector() * 200.f;
                const bool OKA = Nav->ProjectPointToNavigation(Center - Delta, PA, FVector(80,80,250));
                const bool OKB = Nav->ProjectPointToNavigation(Center + Delta, PB, FVector(80,80,250));
                A = PA.Location; B = PB.Location;
                UE_LOG(LogTemp, Display, TEXT("BR_AI_DOOR name=%s projected=%d,%d a=%s b=%s"), *Door->GetName(), OKA, OKB, *A.ToCompactString(), *B.ToCompactString());
                return OKA && OKB && FVector::Dist2D(A, B) > 300.f;
            }
    return false;
}
void ABREntityAISmokeProbe::PlacePawn(APawn* Pawn, FVector Ground, FVector Forward)
{
    auto* Character = Cast<ACharacter>(Pawn);
    const float HalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
    Character->GetCharacterMovement()->StopMovementImmediately();
    Character->SetActorLocationAndRotation(Ground + FVector(0,0,HalfHeight+3.f), Forward.Rotation(), false, nullptr, ETeleportType::TeleportPhysics);
    if (Character->GetController()) Character->GetController()->SetControlRotation(Forward.Rotation());
    if (auto* PC = Cast<APlayerController>(Character->GetController())) PC->ClientSetRotation(Forward.Rotation(), true);
    Character->ForceNetUpdate();
}
void ABREntityAISmokeProbe::PrepareDoorChase()
{
    Controller->UnPossess();
    PlacePawn(Entity, Outside - TowardDoor * 650.f, TowardDoor);
    Entity->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    PlacePawn(Subject, Inside, -TowardDoor);
    Subject->GetCharacterMovement()->DisableMovement();
    Subject->Tags.Remove(TEXT("BR_Hiding"));
    Controller->Possess(Entity);
    Controller->SetControlRotation(TowardDoor.Rotation());
    ServerAttackStart = Entity->GetAttackCueCount();
}
void ABREntityAISmokeProbe::TickLocal()
{
#if !UE_BUILD_SHIPPING
    if (!Entity || !Subject || GetNetMode() == NM_DedicatedServer) return;
    if (LocalStage != Stage)
    {
        LocalStage = Stage; LocalStageStarted = GetWorld()->GetTimeSeconds();
        if (Stage == 5)
        {
            LocalAttackStart = Entity->GetAttackCueCount();
            FString Output;
            if (FParse::Value(FCommandLine::Get(), TEXT("BRAICapture="), Output))
            {
                FApp::SetUnfocusedVolumeMultiplier(1.f); FApp::SetVolumeMultiplier(1.f);
                UAudioMixerBlueprintLibrary::StartRecordingOutput(this, 15.f);
                bRecorded = true;
            }
        }
    }
    if (bRecorded && ((Stage == 5 && GetWorld()->GetTimeSeconds() - LocalStageStarted > 1.5f && !bDoorScreenshot) ||
        (Stage == 6 && GetWorld()->GetTimeSeconds() - LocalStageStarted > .2f && !bAttackScreenshot)))
    {
        FString Output;
        if (FParse::Value(FCommandLine::Get(), TEXT("BRAICapture="), Output))
            FScreenshotRequest::RequestScreenshot(FPaths::Combine(Output, Stage == 5 ? TEXT("door_reopened.png") : TEXT("attack.png")), true, false);
        if (Stage == 5) bDoorScreenshot = true; else bAttackScreenshot = true;
    }
    if (Stage == 6 && GetWorld()->GetTimeSeconds() - LocalStageStarted > 2.f && !bClientAttackChecked)
    {
        bClientAttackChecked = true;
        const bool IsVictim = Subject->IsLocallyControlled();
        const bool OK = Entity->GetAttackCueCount() == LocalAttackStart + 1 && Subject->GetDownedComponent()->IsDowned();
        UE_LOG(LogTemp, Display, TEXT("BR_AI_CLIENT case=ATTACK_ONCE result=%s victim=%d cues=%d"), OK ? TEXT("PASS") : TEXT("FAIL"), IsVictim, Entity->GetAttackCueCount() - LocalAttackStart);
        FString Output;
        if (bRecorded && FParse::Value(FCommandLine::Get(), TEXT("BRAICapture="), Output))
        {
            UAudioMixerBlueprintLibrary::StopRecordingOutput(this, EAudioRecordingExportType::WavFile, TEXT("attack"), Output);
            bRecorded = false;
        }
    }
#endif
}
void ABREntityAISmokeProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    TickLocal();
    if (!HasAuthority() || Stage == 99) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Started) Started = Now;
    if (Now - Started > 240.f) { Check(false, TEXT("TIMEOUT")); Finish(); return; }
    if (Stage == -1)
    {
        for (TActorIterator<ABREntityCharacter> It(GetWorld()); It; ++It) { Entity = *It; break; }
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld()); It; ++It)
        {
            if (!It->IsPlayerControlled()) continue;
            if (!Subject) Subject = *It;
            It->Tags.AddUnique(TEXT("BR_Hiding"));
            It->GetCharacterMovement()->DisableMovement();
        }
        if (!Entity || !Subject || Now - Started < 2) return;
        Controller = Cast<ABREntityAIController>(Entity->GetController());
        if (!Controller) { Check(false, TEXT("CONTROLLER")); Finish(); return; }
        Check(Entity->GetCapsuleComponent()->GetScaledCapsuleRadius() >= 33.9f && Entity->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() >= 71.9f, TEXT("WORLD_COLLISION_SIZE"));
        auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
        Check(Nav && Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)->GetRuntimeGenerationMode() == ERuntimeGenerationType::Dynamic, TEXT("DYNAMIC_NAVIGATION"));
        for (TActorIterator<ABRGarageDoor> It(GetWorld()); It; ++It)
            if (!It->IsA<ABRGarageExitDoor>() && !It->IsA<ABRLootCabinet>()) Doors.Add(*It);
        Doors.Sort([](const ABRGarageDoor& A, const ABRGarageDoor& B) { return A.GetName() < B.GetName(); });
        Check(Doors.Num() >= 6, TEXT("SAVED_MAP_DOORS"));
        Controller->UnPossess(); Controller->Possess(Entity);
        PatrolOrigin = LastSample = Entity->GetActorLocation(); LastGoal = FVector::ZeroVector;
        SetStage(0); return;
    }
    if (Stage == 0)
    {
        if (Now >= NextSample)
        {
            NextSample = Now + 1.f;
            const FVector Position = Entity->GetActorLocation();
            Travel += FVector::Dist2D(Position, LastSample); LastSample = Position;
            MaxPatrolDistance = FMath::Max(MaxPatrolDistance, float(FVector::Dist2D(Position, PatrolOrigin)));
            PatrolCells.Add(FIntPoint(FMath::FloorToInt(Position.X/1000), FMath::FloorToInt(Position.Y/1000)));
            const auto Path = Controller->GetPathFollowingComponent()->GetPath();
            if (Path.IsValid() && Path->IsValid() && FVector::Dist2D(LastGoal, Path->GetEndLocation()) > 500)
            {
                LastGoal = Path->GetEndLocation(); ++GoalCount;
                const float Distance = FVector::Dist2D(Position, LastGoal);
                MinGoalDistance = FMath::Min(MinGoalDistance, Distance);
                Check(!Path->IsPartial(), TEXT("PATROL_FULL_PATH"));
            }
            UE_LOG(LogTemp, Display, TEXT("BR_AI_SAMPLE location=%s speed=%.1f traveled=%.1f goals=%d"), *Position.ToCompactString(), Entity->GetVelocity().Size2D(), Travel, GoalCount);
        }
        const bool bDoorsOnly = FParse::Param(FCommandLine::Get(), TEXT("BRAIDoorsOnly"));
        if (!bDoorsOnly && Now - StageStarted < 75.f) return;
        if (!bDoorsOnly)
        {
            Check(Travel > 6000.f && MaxPatrolDistance > 2500.f && PatrolCells.Num() >= 5, TEXT("PATROL_COVERS_GARAGE"));
            Check(GoalCount >= 2 && MinGoalDistance > 1200.f, TEXT("DISTANT_PATROL_GOALS"));
        }
        Controller->UnPossess();
        PlacePawn(Entity, PatrolOrigin, FVector::ForwardVector);
        Entity->GetCharacterMovement()->DisableMovement();
        if (Doors.IsEmpty()) { Finish(); return; }
        DoorIndex = 0; Doors[0]->SetUnlockable(true); SetStage(1); return;
    }
    if (Stage == 1 && Now - StageStarted > 2.f)
    {
        FVector A, B;
        const bool PointsOK = DoorPoints(Doors[DoorIndex], A, B);
        DoorA = A; DoorB = B;
        Check(PointsOK, *FString::Printf(TEXT("DOOR_%d_NAV_POINTS"), DoorIndex));
        Check(PointsOK && !HasFullPath(A, B), *FString::Printf(TEXT("DOOR_%d_CLOSED_BLOCKS_PATH"), DoorIndex));
        if (!ChaseDoor && PointsOK)
        {
            ChaseDoor = Doors[DoorIndex];
            const bool AOutside = HasFullPath(PatrolOrigin, A);
            Outside = AOutside ? A : B; Inside = AOutside ? B : A;
            DoorCenter = (A + B) * .5f; TowardDoor = (Inside - Outside).GetSafeNormal2D();
        }
        Doors[DoorIndex]->SetDoorOpen(true); SetStage(2); return;
    }
    if (Stage == 2 && Now - StageStarted > 3.f)
    {
        // Query the same doorway endpoints, not the rotated leaf's new bounds.
        UE_LOG(LogTemp, Display, TEXT("BR_AI_DOOR_OPEN name=%s alpha=%.2f a=%s b=%s"), *Doors[DoorIndex]->GetName(), Doors[DoorIndex]->GetOpenAlpha(), *DoorA.ToCompactString(), *DoorB.ToCompactString());
        Check(HasFullPath(DoorA, DoorB), *FString::Printf(TEXT("DOOR_%d_OPEN_RESTORES_PATH"), DoorIndex));
        Doors[DoorIndex]->SetDoorOpen(false);
        if (++DoorIndex < Doors.Num()) { Doors[DoorIndex]->SetUnlockable(true); SetStage(1); }
        else
        {
            if (!ChaseDoor) { Finish(); return; }
            ChaseDoor->SetDoorOpen(true);
            SetStage(3);
        }
        return;
    }
    if (Stage == 3)
    {
        if (Now - StageStarted < 2.f) return;
        if (Entity->GetController() != Controller) { PrepareDoorChase(); return; }
        if (Entity->GetEntityState() != EBREntityState::Chasing)
        {
            if (Now - StageStarted > 7.f) { Check(false, TEXT("SEES_PLAYER_THROUGH_OPEN_DOOR")); Finish(); }
            return;
        }
        Check(Entity->GetTargetActor() == Subject && Subject->GetStaminaComponent()->HasUnlimitedStamina(), TEXT("REAL_SIGHT_STARTS_CHASE"));
        Check(ChaseDoor->SetDoorOpen(false), TEXT("PLAYER_CLOSES_DOOR_DURING_CHASE"));
        SetStage(4); return;
    }
    if (Stage == 4)
    {
        const double Elapsed = Now - StageStarted;
        if (!bCheckedGrace && Elapsed > 2.f)
        {
            bCheckedGrace = true;
            Check(Entity->GetEntityState() == EBREntityState::Chasing && !Subject->GetDownedComponent()->IsDowned(), TEXT("CLOSED_DOOR_SIGHT_GRACE"));
        }
        if (Entity->GetEntityState() != EBREntityState::Chasing)
        {
            Check(Elapsed >= 5.5f && Elapsed <= 9.5f && !Subject->GetDownedComponent()->IsDowned(), TEXT("CHASE_EXPIRES_AFTER_LOST_SIGHT"));
            Check(!Subject->GetStaminaComponent()->HasUnlimitedStamina(), TEXT("CHASE_STAMINA_RELEASED"));
            ReturnOrigin = Entity->GetActorLocation(); SetStage(40); return;
        }
        if (Elapsed > 10.f) { Check(false, TEXT("CHASE_EXPIRES_AFTER_LOST_SIGHT")); Finish(); }
        return;
    }
    if (Stage == 40 && Now - StageStarted > 18.f)
    {
        const float DoorDistance = FVector::Dist2D(Entity->GetActorLocation(), DoorCenter);
        const float ReturnDistance = FVector::Dist2D(Entity->GetActorLocation(), ReturnOrigin);
        UE_LOG(LogTemp, Display, TEXT("BR_AI_RETREAT door_distance=%.1f displacement=%.1f location=%s"), DoorDistance, ReturnDistance, *Entity->GetActorLocation().ToCompactString());
        Check(DoorDistance > 1400 && ReturnDistance > 1000 && !Subject->GetDownedComponent()->IsDowned(), TEXT("WALKS_AWAY_FROM_CLOSED_DOOR"));
        Subject->Tags.AddUnique(TEXT("BR_Hiding"));
        Controller->UnPossess();
        ChaseDoor->SetDoorOpen(true);
        PlacePawn(Entity, Outside - TowardDoor * 450.f, TowardDoor);
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld()); It; ++It)
            if (*It != Subject && It->IsPlayerControlled())
            {
                const FVector Ground = Outside - TowardDoor * 200.f + FVector::CrossProduct(FVector::UpVector, TowardDoor) * 90.f;
                PlacePawn(*It, Ground, (Inside - Ground).GetSafeNormal2D());
            }
        SetStage(5); return;
    }
    if (Stage == 5)
    {
        if (Now - StageStarted < 2.f) return;
        if (Now >= NextSample)
        {
            NextSample = Now + 1.f;
            const auto* Capsule = Entity->GetCapsuleComponent();
            FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(BR_AI_MOVE_SWEEP), false, Entity);
            GetWorld()->SweepSingleByChannel(Hit, Entity->GetActorLocation(), Entity->GetActorLocation()+TowardDoor*200.f, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params);
            const auto Path = Controller->GetPathFollowingComponent()->GetPath();
            UE_LOG(LogTemp, Display, TEXT("BR_AI_APPROACH location=%s speed=%.1f radius=%.1f half=%.1f movement=%d pathstatus=%d endpoint=%s hit=%s component=%s impact=%s"), *Entity->GetActorLocation().ToCompactString(), Entity->GetVelocity().Size2D(), Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight(), int32(Entity->GetCharacterMovement()->MovementMode), int32(Controller->GetMoveStatus()), Path.IsValid() ? *Path->GetEndLocation().ToCompactString() : TEXT("none"), *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), *Hit.ImpactPoint.ToCompactString());
        }
        if (Entity->GetController() != Controller)
        {
            PlacePawn(Subject, Inside, -TowardDoor);
            Subject->Tags.Remove(TEXT("BR_Hiding"));
            Check(ChaseDoor->GetOpenAlpha() > .99f && HasFullPath(Outside, Inside), TEXT("REOPENED_DOOR_NAV_READY"));
            Entity->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
            Controller->Possess(Entity); Controller->SetControlRotation(TowardDoor.Rotation());
            return;
        }
        if (Subject->GetDownedComponent()->IsDowned())
        {
            Check(Entity->GetAttackCueCount() == ServerAttackStart + 1, TEXT("ATTACK_ONCE_ON_SERVER"));
            Check(FVector::DotProduct(Entity->GetActorLocation() - DoorCenter, TowardDoor) > 25.f, TEXT("WALKS_THROUGH_REOPENED_DOOR"));
            SetStage(6); return;
        }
        if (Now - StageStarted > 15.f)
        {
            UE_LOG(LogTemp, Display, TEXT("BR_AI_BLOCKED door_alpha=%.2f entity=%s subject=%s nav=%d"), ChaseDoor->GetOpenAlpha(), *Entity->GetActorLocation().ToCompactString(), *Subject->GetActorLocation().ToCompactString(), HasFullPath(Outside, Inside));
            FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(BR_AI_DOOR_SWEEP), false, Entity);
            const auto* Capsule = Entity->GetCapsuleComponent();
            const FVector Lift(0,0,Capsule->GetScaledCapsuleHalfHeight()+3);
            GetWorld()->SweepSingleByChannel(Hit, Outside+Lift, Inside+Lift, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(Capsule->GetScaledCapsuleRadius(), Capsule->GetScaledCapsuleHalfHeight()), Params);
            UE_LOG(LogTemp, Display, TEXT("BR_AI_BLOCKED sweep_actor=%s component=%s point=%s"), *GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()), *Hit.ImpactPoint.ToCompactString());
            for (TActorIterator<ABRPlayerCharacter> It(GetWorld()); It; ++It) UE_LOG(LogTemp, Display, TEXT("BR_AI_BLOCKED player=%s location=%s"), *It->GetName(), *It->GetActorLocation().ToCompactString());
            Check(false, TEXT("REOPENED_DOOR_ATTACK")); Finish();
        }
        return;
    }
    if (Stage == 6 && Now - StageStarted > 4.f)
    {
        Check(Entity->GetAttackCueCount() == ServerAttackStart + 1, TEXT("NO_REPEAT_ATTACK_ON_DOWNED_PLAYER"));
        Finish();
    }
#endif
}
