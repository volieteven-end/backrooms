#include "World/BRExtractionZone.h"

#include "Components/BoxComponent.h"
#include "Core/BRGameMode.h"
#include "Core/BRGameState.h"
#include "Core/BRGameplayRules.h"
#include "GameFramework/Pawn.h"
#include "Player/BRPlayerState.h"
#include "TimerManager.h"
#include "World/BRGarageExitDoor.h"

ABRExtractionZone::ABRExtractionZone()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = false;
	Zone = CreateDefaultSubobject<UBoxComponent>(TEXT("ExtractionZone"));
	Zone->SetBoxExtent(FVector(200.0f));
	Zone->SetCollisionProfileName(TEXT("Trigger"));
	SetRootComponent(Zone);
}

void ABRExtractionZone::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		Zone->OnComponentBeginOverlap.AddDynamic(this, &ABRExtractionZone::OnZoneBeginOverlap);
		Zone->OnComponentEndOverlap.AddDynamic(this, &ABRExtractionZone::OnZoneEndOverlap);
		GetWorldTimerManager().SetTimer(ExtractionCheckTimer, this, &ABRExtractionZone::CheckExtractionState, 0.5f, true);
	}
}

void ABRExtractionZone::OnZoneBeginOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (Pawn->IsPlayerControlled())
		{
			OverlappingPlayers.Add(Pawn);
			CheckExtractionState();
		}
	}
}

void ABRExtractionZone::OnZoneEndOverlap(UPrimitiveComponent*, AActor* OtherActor, UPrimitiveComponent*, int32)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		OverlappingPlayers.Remove(Pawn);
	}
}

void ABRExtractionZone::CheckExtractionState()
{
	if (!HasAuthority() || bTriggered)
	{
		return;
	}

	const ABRGameState* State = GetWorld()->GetGameState<ABRGameState>();
	if (!State || (RequiredExitDoor && !RequiredExitDoor->IsPassageOpen()) || State->GetLevelPhase()==EBRLevelPhase::Completed || State->GetLevelPhase()==EBRLevelPhase::Failed)
	{
		return;
	}

	int32 ActivePlayers = 0;
	int32 PlayersInZone = 0;
	for (APlayerState* BasePlayerState : State->PlayerArray)
	{
		const ABRPlayerState* PlayerState = Cast<ABRPlayerState>(BasePlayerState);
		APawn* Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
		if (PlayerState && Pawn && !PlayerState->IsDowned())
		{
			++ActivePlayers;
			if (OverlappingPlayers.Contains(Pawn))
			{
				++PlayersInZone;
			}
		}
	}

	if (FBRGameplayRules::CanExtract(bRequireAllPlayers?ActivePlayers:1, PlayersInZone, State->AreObjectivesComplete()))
	{
		bTriggered = true;
		OnTeamReadyToExtract();
		if (ABRGameMode* GameMode = Cast<ABRGameMode>(GetWorld()->GetAuthGameMode()))
		{
			GameMode->HandleTeamExtracted(NextMapName);
		}
	}
}
