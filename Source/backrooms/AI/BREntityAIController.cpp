#include "AI/BREntityAIController.h"

#include "AI/BREntityCharacter.h"
#include "Core/BRGameState.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "World/BRGarageDoor.h"
#include "World/BRLootCabinet.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "TimerManager.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Hearing.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"

ABREntityAIController::ABREntityAIController()
{
    EntityPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("EntityPerception"));
    SetPerceptionComponent(*EntityPerception);
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 10000.f;
    SightConfig->LoseSightRadius = 10500.f;
    SightConfig->PeripheralVisionAngleDegrees = 60.f;
    SightConfig->SetMaxAge(8.f);
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
    HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
    HearingConfig->HearingRange = 550.f;
    HearingConfig->SetMaxAge(6.f);
    HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
    HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
    HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;
    EntityPerception->ConfigureSense(*SightConfig);
    EntityPerception->ConfigureSense(*HearingConfig);
    EntityPerception->SetDominantSense(UAISense_Sight::StaticClass());
    EntityPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ThisClass::OnTargetPerceptionUpdated);
}

void ABREntityAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
    bTraceBehavior = FParse::Param(FCommandLine::Get(), TEXT("BREntityAILog"));
    RecentLocations.Reset();
    UnreachableTarget.Reset();
    NextMoveAttempt = AttackCooldown = AvoidLocationUntil = 0;
    LastProgressTime = GetWorld()->GetTimeSeconds();
    LastProgressLocation = InPawn->GetActorLocation();
    bMoveFailed = false;
    if (auto* Entity = Cast<ABREntityCharacter>(InPawn))
    {
        Entity->GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
        Entity->SetEntityState(EBREntityState::Patrolling);
    }
    GetWorldTimerManager().SetTimer(BehaviorTimer, this, &ThisClass::UpdateBehavior, .2f, true);
}

void ABREntityAIController::OnUnPossess()
{
    GetWorldTimerManager().ClearTimer(BehaviorTimer);
    if (auto* Entity = Cast<ABREntityCharacter>(GetPawn())) Entity->SetEntityState(EBREntityState::Idle);
    Super::OnUnPossess();
}

bool ABREntityAIController::IsEligibleTarget(const ABRPlayerCharacter* Player) const
{
    return IsValid(Player) && !Player->GetDownedComponent()->IsDowned() &&
        !Player->GetCurrentHideSpot() && !Player->ActorHasTag(TEXT("BR_Hiding")) &&
        (Player != UnreachableTarget.Get() || GetWorld()->GetTimeSeconds() >= UnreachableTargetUntil);
}

bool ABREntityAIController::CanCurrentlySee(ABRPlayerCharacter* Player) const
{
    if (!IsEligibleTarget(Player)) return false;
    FActorPerceptionBlueprintInfo Info;
    if (!EntityPerception->GetActorsPerception(Player, Info)) return false;
    for (const FAIStimulus& Stimulus : Info.LastSensedStimuli)
        if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>() && Stimulus.WasSuccessfullySensed())
            return LineOfSightTo(Player);
    return false;
}

void ABREntityAIController::StartChase(ABRPlayerCharacter* Player)
{
    auto* Entity = Cast<ABREntityCharacter>(GetPawn());
    if (!Entity || !IsEligibleTarget(Player)) return;
    const bool bNewTarget = Entity->GetEntityState() != EBREntityState::Chasing || Entity->GetTargetActor() != Player;
    LastSeenTime = GetWorld()->GetTimeSeconds();
    LastSeenLocation = Player->GetActorLocation();
    if (!bNewTarget) return;
    StopMovement();
    LastProgressTime = LastSeenTime;
    LastProgressLocation = Entity->GetActorLocation();
    NextMoveAttempt = 0;
    bMoveFailed = false;
    Entity->SetEntityState(EBREntityState::Chasing, Player);
    TraceBehavior(TEXT("CHASE_STARTED"));
}

void ABREntityAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
    auto* Entity = Cast<ABREntityCharacter>(GetPawn());
    auto* Player = Cast<ABRPlayerCharacter>(Actor);
    if (!Entity || !HasAuthority() || !IsEligibleTarget(Player)) return;
    if (GetWorld()->GetTimeSeconds() < AttackCooldown) return;
    if (const auto* State = GetWorld()->GetGameState<ABRGameState>())
        if (State->GetLevelPhase() != EBRLevelPhase::Exploring && State->GetLevelPhase() != EBRLevelPhase::ExtractionReady) return;
    // A failed hearing stimulus must not cancel vision. Lost sight is handled by
    // the explicit grace timer, refreshed while the player remains visible.
    if (!Stimulus.WasSuccessfullySensed()) return;
    auto* Current = Cast<ABRPlayerCharacter>(Entity->GetTargetActor());
    if (Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>())
    {
        if (!Current || Current == Player || !CanCurrentlySee(Current)) StartChase(Player);
    }
    else if (Stimulus.Type == UAISense::GetSenseID<UAISense_Hearing>() && Entity->GetEntityState() != EBREntityState::Chasing)
    {
        StopMovement();
        Entity->SetEntityState(EBREntityState::Investigating);
        LastSeenLocation = Stimulus.StimulusLocation;
        InvestigationEnds = GetWorld()->GetTimeSeconds() + SightMemorySeconds;
        NextMoveAttempt = 0;
        LastProgressTime = GetWorld()->GetTimeSeconds();
        LastProgressLocation = Entity->GetActorLocation();
        bMoveFailed = false;
    }
}

void ABREntityAIController::RememberLocation(const FVector& Location)
{
    RecentLocations.Add(Location);
    if (RecentLocations.Num() > 6) RecentLocations.RemoveAt(0);
}

void ABREntityAIController::RememberBlockedLocation()
{
    const APawn* Entity = GetPawn();
    if (!Entity) return;
    AvoidLocation = Entity->GetActorLocation();
    float Nearest = 700.f;
    for (TActorIterator<ABRGarageDoor> Door(GetWorld()); Door; ++Door)
        if (!Door->IsA<ABRLootCabinet>() && Door->GetOpenAlpha() < .95f && FVector::Dist2D(Entity->GetActorLocation(), Door->GetActorLocation()) < Nearest)
        {
            Nearest = FVector::Dist2D(Entity->GetActorLocation(), Door->GetActorLocation());
            AvoidLocation = Door->GetActorLocation();
        }
    AvoidLocationUntil = GetWorld()->GetTimeSeconds() + 25.f;
}

void ABREntityAIController::BeginReturn(bool bUnreachable)
{
    auto* Entity = Cast<ABREntityCharacter>(GetPawn());
    if (!Entity) return;
    if (bUnreachable)
    {
        UnreachableTarget = Cast<ABRPlayerCharacter>(Entity->GetTargetActor());
        UnreachableTargetUntil = GetWorld()->GetTimeSeconds() + 10.f;
    }
    RememberBlockedLocation();
    ReturnStart = Entity->GetActorLocation();
    RememberLocation(ReturnStart);
    StopMovement();
    Entity->SetEntityState(EBREntityState::Returning);
    Entity->GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
    LastProgressLocation = ReturnStart;
    LastProgressTime = GetWorld()->GetTimeSeconds();
    NextMoveAttempt = 0;
    bMoveFailed = false;
    TraceBehavior(bUnreachable ? TEXT("BLOCKED_RETURN") : TEXT("LOST_TARGET_RETURN"));
}

bool ABREntityAIController::ChoosePatrolGoal(FVector& OutLocation)
{
    auto* Entity = Cast<ABREntityCharacter>(GetPawn());
    auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
    ANavigationData* Data = Nav && Entity ? Nav->GetNavDataForProps(Entity->GetNavAgentPropertiesRef(), Entity->GetNavAgentLocation()) : nullptr;
    if (!Data) return false;
    const FVector Origin = Entity->GetNavAgentLocation();
    const double Now = GetWorld()->GetTimeSeconds();
    const bool bReturning = Entity->GetEntityState() == EBREntityState::Returning;
    float BestScore = -FLT_MAX;
    bool bFound = false;
    for (int32 I = 0; I < 32; ++I)
    {
        FNavLocation Candidate;
        if (!Nav->GetRandomReachablePointInRadius(Origin, PatrolRadius, Candidate, Data)) continue;
        const float Distance = FVector::Dist2D(Origin, Candidate.Location);
        if (Distance < 200.f) continue;
        const float FromBlocked = FVector::Dist2D(AvoidLocation, Candidate.Location);
        if (Now < AvoidLocationUntil && FromBlocked < 800.f) continue;
        FPathFindingQuery Query(this, *Data, Origin, Candidate.Location);
        Query.SetAllowPartialPaths(false);
        const FPathFindingResult Path = Nav->FindPathSync(Entity->GetNavAgentPropertiesRef(), Query);
        if (!Path.IsSuccessful() || !Path.Path.IsValid() || Path.Path->IsPartial() || Path.Path->GetPathPoints().Num() < 2) continue;
        float Novelty = 2000.f;
        for (const FVector& Previous : RecentLocations) Novelty = FMath::Min(Novelty, float(FVector::Dist2D(Previous, Candidate.Location)));
        float Score = FMath::Min(Distance, 5000.f) + Novelty + FMath::FRandRange(0, 2200.f);
        if (Distance >= MinPatrolDistance) Score += 10000.f;
        if (bReturning)
        {
            if (FVector::Dist2D(ReturnStart, Candidate.Location) >= RetreatDistance) Score += 10000.f;
            Score += FMath::Min(FromBlocked, 4000.f);
        }
        if (Score > BestScore) { BestScore = Score; OutLocation = Candidate.Location; bFound = true; }
    }
    return bFound;
}

void ABREntityAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);
    auto* Entity = Cast<ABREntityCharacter>(GetPawn());
    if (!Entity || Result.Code == EPathFollowingResult::Aborted) return;
    if (Result.IsSuccess())
    {
        bMoveFailed = false;
        if (Entity->GetEntityState() == EBREntityState::Patrolling || Entity->GetEntityState() == EBREntityState::Returning)
        {
            RememberLocation(Entity->GetActorLocation());
            if (Entity->GetEntityState() == EBREntityState::Returning && FVector::Dist2D(ReturnStart, Entity->GetActorLocation()) >= RetreatDistance - 100.f)
                Entity->SetEntityState(EBREntityState::Patrolling);
            NextMoveAttempt = GetWorld()->GetTimeSeconds() + .3f;
            TraceBehavior(TEXT("PATROL_ARRIVED"));
        }
    }
    else
    {
        bMoveFailed = true;
        NextMoveAttempt = GetWorld()->GetTimeSeconds() + .8f;
        TraceBehavior(TEXT("MOVE_FAILED"));
    }
}

void ABREntityAIController::UpdateBehavior()
{
    auto* Entity = Cast<ABREntityCharacter>(GetPawn());
    if (!Entity || !HasAuthority()) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (const auto* State = GetWorld()->GetGameState<ABRGameState>())
        if (State->GetLevelPhase() != EBRLevelPhase::Exploring && State->GetLevelPhase() != EBRLevelPhase::ExtractionReady)
        {
            StopMovement();
            if (Entity->GetEntityState() != EBREntityState::Idle) Entity->SetEntityState(EBREntityState::Idle);
            return;
        }
    if (Now < AttackCooldown) return;
    auto* Target = Cast<ABRPlayerCharacter>(Entity->GetTargetActor());
    if (Target && !IsEligibleTarget(Target)) { BeginReturn(false); Target = nullptr; }
    if (!Target)
    {
        TArray<AActor*> Seen;
        EntityPerception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(), Seen);
        ABRPlayerCharacter* Closest = nullptr;
        for (AActor* Actor : Seen)
            if (auto* Player = Cast<ABRPlayerCharacter>(Actor); CanCurrentlySee(Player) && (!Closest || Entity->GetDistanceTo(Player) < Entity->GetDistanceTo(Closest))) Closest = Player;
        if (Closest) { StartChase(Closest); Target = Closest; }
    }
    if (FVector::Dist2D(Entity->GetActorLocation(), LastProgressLocation) > 30.f)
    {
        LastProgressLocation = Entity->GetActorLocation();
        LastProgressTime = Now;
        bMoveFailed = false;
    }
    if (Target && Entity->GetEntityState() == EBREntityState::Chasing)
    {
        const bool bVisible = CanCurrentlySee(Target);
        if (bVisible) { LastSeenTime = Now; LastSeenLocation = Target->GetActorLocation(); }
        else if (Now - LastSeenTime >= SightMemorySeconds) { BeginReturn(false); return; }
        // Half-second acceleration, matching the source sprint transition.
        Entity->GetCharacterMovement()->MaxWalkSpeed = FMath::FInterpConstantTo(Entity->GetCharacterMovement()->MaxWalkSpeed, ChaseSpeed, .2f, (ChaseSpeed - PatrolSpeed) / .5f);
        if (Entity->GetDistanceTo(Target) < 130.f && LineOfSightTo(Target))
        {
            AttackCooldown = Now + FMath::Max(1.5f, Entity->GetAttackAnimationDuration());
            Entity->PlayReplicatedAttack();
            Target->GetDownedComponent()->Down();
            BeginReturn(false);
            TraceBehavior(TEXT("ATTACK"));
            return;
        }
        if (Now - LastProgressTime >= StuckSeconds)
        {
            if (bVisible) { BeginReturn(true); return; }
            // Keep only the remaining sight grace at a closed door, then leave.
            if (GetMoveStatus() != EPathFollowingStatus::Idle) StopMovement();
            bMoveFailed = true;
        }
        if (GetMoveStatus() == EPathFollowingStatus::Idle && Now >= NextMoveAttempt && !(bMoveFailed && !bVisible))
        {
            const auto Result = MoveToActor(Target, 20.f, true, true, false, nullptr, false);
            bMoveFailed = Result != EPathFollowingRequestResult::RequestSuccessful;
            NextMoveAttempt = Now + .8f;
        }
        return;
    }
    Entity->GetCharacterMovement()->MaxWalkSpeed = PatrolSpeed;
    if (Entity->GetEntityState() == EBREntityState::Investigating)
    {
        if (Now >= InvestigationEnds || bMoveFailed) { BeginReturn(false); return; }
        if (GetMoveStatus() == EPathFollowingStatus::Idle && Now >= NextMoveAttempt)
        {
            const auto Result = MoveToLocation(LastSeenLocation, 80.f, false, true, true, false, nullptr, false);
            if (Result != EPathFollowingRequestResult::RequestSuccessful) { BeginReturn(false); return; }
            NextMoveAttempt = Now + .8f;
        }
        return;
    }
    if (GetMoveStatus() == EPathFollowingStatus::Moving && Now - LastProgressTime >= StuckSeconds)
    {
        RememberBlockedLocation();
        StopMovement();
        NextMoveAttempt = Now + .5f;
        TraceBehavior(TEXT("PATROL_REPLAN"));
    }
    if (GetMoveStatus() != EPathFollowingStatus::Idle || Now < NextMoveAttempt) return;
    if (bMoveFailed) { RememberBlockedLocation(); bMoveFailed = false; }
    FVector Goal;
    NextMoveAttempt = Now + 1.f;
    if (ChoosePatrolGoal(Goal))
    {
        if (Entity->GetEntityState() != EBREntityState::Returning) Entity->SetEntityState(EBREntityState::Patrolling);
        const auto Result = MoveToLocation(Goal, 60.f, false, true, false, false, nullptr, false);
        bMoveFailed = Result == EPathFollowingRequestResult::Failed;
        LastProgressLocation = Entity->GetActorLocation();
        LastProgressTime = Now;
        if (bTraceBehavior) UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_AI event=PATROL_GOAL time=%.2f distance=%.1f goal=%s complete_path=%d"), Now, FVector::Dist2D(Goal, Entity->GetActorLocation()), *Goal.ToCompactString(), Result == EPathFollowingRequestResult::RequestSuccessful);
    }
}

void ABREntityAIController::TraceBehavior(const TCHAR* Event) const
{
    if (bTraceBehavior && GetPawn()) UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_AI event=%s time=%.2f location=%s"), Event, GetWorld()->GetTimeSeconds(), *GetPawn()->GetActorLocation().ToCompactString());
}
