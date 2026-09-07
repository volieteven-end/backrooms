#include "World/BRObjectiveActor.h"

#include "Components/StaticMeshComponent.h"
#include "Core/BRGameState.h"
#include "Net/UnrealNetwork.h"

ABRObjectiveActor::ABRObjectiveActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);
}

void ABRObjectiveActor::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		if (ABRGameState* State = GetWorld()->GetGameState<ABRGameState>())
		{
			State->RegisterObjective();
		}
	}
}

bool ABRObjectiveActor::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return !bCompleted && IsValid(InstigatorPawn);
}

FText ABRObjectiveActor::GetInteractionText_Implementation(APawn* InstigatorPawn) const
{
	return CanInteract_Implementation(InstigatorPawn) ? InteractionText : FText::GetEmpty();
}

void ABRObjectiveActor::Interact_Implementation(APawn* InstigatorPawn)
{
	CompleteObjective(InstigatorPawn);
}

void ABRObjectiveActor::CompleteObjective(APawn* CompletingPawn)
{
	if (!HasAuthority() || bCompleted || !IsValid(CompletingPawn))
	{
		return;
	}

	bCompleted = true;
	if (ABRGameState* State = GetWorld()->GetGameState<ABRGameState>())
	{
		State->NotifyObjectiveCompleted();
	}
	OnObjectiveCompleted(CompletingPawn);
	OnObjectiveStateChanged(true);
	ForceNetUpdate();
}

void ABRObjectiveActor::OnRep_Completed()
{
	OnObjectiveStateChanged(bCompleted);
}

void ABRObjectiveActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABRObjectiveActor, bCompleted);
}
