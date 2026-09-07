#include "AI/BREntityAIController.h"

#include "AI/BREntityCharacter.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
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
	SightConfig->SightRadius = 1600.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 70.0f;
	SightConfig->SetMaxAge(4.0f);
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;

	HearingConfig = CreateDefaultSubobject<UAISenseConfig_Hearing>(TEXT("HearingConfig"));
	HearingConfig->HearingRange = 2200.0f;
	HearingConfig->SetMaxAge(6.0f);
	HearingConfig->DetectionByAffiliation.bDetectEnemies = true;
	HearingConfig->DetectionByAffiliation.bDetectFriendlies = true;
	HearingConfig->DetectionByAffiliation.bDetectNeutrals = true;

	EntityPerception->ConfigureSense(*SightConfig);
	EntityPerception->ConfigureSense(*HearingConfig);
	EntityPerception->SetDominantSense(UAISense_Sight::StaticClass());
	EntityPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ABREntityAIController::OnTargetPerceptionUpdated);
}

void ABREntityAIController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
    GetWorldTimerManager().SetTimer(BehaviorTimer, this, &ThisClass::UpdateBehavior, 0.35f, true);
	if (ABREntityCharacter* Entity = Cast<ABREntityCharacter>(InPawn))
	{
		Entity->SetEntityState(EBREntityState::Patrolling);
	}
}

void ABREntityAIController::OnTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	ABREntityCharacter* Entity = Cast<ABREntityCharacter>(GetPawn());
	if (!Entity || !Cast<ABRPlayerCharacter>(Actor) || Cast<ABRPlayerCharacter>(Actor)->GetDownedComponent()->IsDowned())
	{
		return;
	}

	if (Actor->ActorHasTag(TEXT("BR_Hiding")))
	{
		EntityPerception->ForgetActor(Actor);
		if (Entity->GetTargetActor() == Actor)
		{
			StopMovement();
			Entity->SetEntityState(EBREntityState::Returning);
		}
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		const bool bWasSeen = Stimulus.Type == UAISense::GetSenseID<UAISense_Sight>();
		Entity->SetEntityState(bWasSeen ? EBREntityState::Chasing : EBREntityState::Investigating, Actor);
		if (bWasSeen)
		{
			MoveToActor(Actor, 90.0f, true, true, true, nullptr, true);
		}
		else
		{
			MoveToLocation(Stimulus.StimulusLocation, 120.0f, true, true, true, false);
		}
	}
	else if (Entity->GetTargetActor() == Actor)
	{
		StopMovement();
		Entity->SetEntityState(EBREntityState::Returning);
	}
}

void ABREntityAIController::OnUnPossess()
{
    GetWorldTimerManager().ClearTimer(BehaviorTimer); Super::OnUnPossess();
}
void ABREntityAIController::UpdateBehavior()
{
    auto* Entity = Cast<ABREntityCharacter>(GetPawn()); if (!Entity || !HasAuthority()) return;
    if (GetWorld()->GetTimeSeconds()<AttackCooldown) return;
    auto* Target = Cast<ABRPlayerCharacter>(Entity->GetTargetActor());
    if (Target && (Target->GetCurrentHideSpot() || Target->GetDownedComponent()->IsDowned()))
    { EntityPerception->ForgetActor(Target); StopMovement(); Entity->SetEntityState(EBREntityState::Returning); Target = nullptr; }
    if (Target && Entity->GetEntityState() == EBREntityState::Chasing)
    {
        Entity->GetCharacterMovement()->MaxWalkSpeed = 520.0f;
        if (Entity->GetDistanceTo(Target) < 130.0f && LineOfSightTo(Target) && GetWorld()->GetTimeSeconds() >= AttackCooldown)
        { AttackCooldown = GetWorld()->GetTimeSeconds() + 1.5f; Entity->PlayReplicatedAttack(); Target->GetDownedComponent()->Down(); StopMovement(); Entity->SetEntityState(EBREntityState::Returning); }
        else if (GetMoveStatus() == EPathFollowingStatus::Idle) MoveToActor(Target,90.0f);
        return;
    }
    if (GetMoveStatus() == EPathFollowingStatus::Idle)
    {
        Entity->GetCharacterMovement()->MaxWalkSpeed = 200.0f;
        if (auto* Nav = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld()))
        {
            FNavLocation Point;
            if (Nav->GetRandomReachablePointInRadius(Entity->GetActorLocation(),900.0f,Point))
            { Entity->SetEntityState(EBREntityState::Patrolling); MoveToLocation(Point.Location,60.0f); }
        }
    }
}
