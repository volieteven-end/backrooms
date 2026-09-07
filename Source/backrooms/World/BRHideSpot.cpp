#include "World/BRHideSpot.h"
#include "World/BRLootCabinet.h"
#include "Components/CapsuleComponent.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

ABRHideSpot::ABRHideSpot()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
    bAlwaysRelevant = true;
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	InteractionBox->SetBoxExtent(FVector(80.0f, 80.0f, 120.0f));
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	SetRootComponent(InteractionBox);
}

void ABRHideSpot::BeginPlay()
{Super::BeginPlay();if(Cabinet){InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);SetActorHiddenInGame(true);}}

bool ABRHideSpot::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	const auto* Character = Cast<ABRPlayerCharacter>(InstigatorPawn);
    return Character && !Character->GetDownedComponent()->IsDowned() &&
        !Cabinet &&
        (!Character->GetCurrentHideSpot() || Character->GetCurrentHideSpot() == this) &&
        (!IsValid(Occupant) || Occupant.Get() == InstigatorPawn);
}

FText ABRHideSpot::GetInteractionText_Implementation(APawn* InstigatorPawn) const
{
	if (!CanInteract_Implementation(InstigatorPawn))
	{
		return FText::GetEmpty();
	}
	return Occupant.Get() == InstigatorPawn
		? NSLOCTEXT("Backrooms", "LeaveHideSpot", "离开躲藏点")
		: NSLOCTEXT("Backrooms", "EnterHideSpot", "躲藏");
}

void ABRHideSpot::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !CanInteract_Implementation(InstigatorPawn))
	{
		return;
	}
	if (Occupant.Get() == InstigatorPawn)
	{
		Exit(InstigatorPawn);
	}
	else
	{
		Enter(InstigatorPawn);
	}
}

void ABRHideSpot::Enter(APawn* Pawn)
{
    auto* Character = Cast<ABRPlayerCharacter>(Pawn); if (!Character) return;
    EntryLocation = Pawn->GetActorLocation(); Occupant = Pawn;
    Character->SetCurrentHideSpot(this);
    Pawn->SetActorLocation(GetActorLocation()+FVector(0,0,Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), false, nullptr, ETeleportType::TeleportPhysics);
    if(Cabinet)Cabinet->SetDoorOpen(false);
    ForceNetUpdate();
}
void ABRHideSpot::Exit(APawn* Pawn)
{
    FVector Location = EntryLocation;
    FRotator Rotation = Pawn->GetActorRotation();
    if (!GetWorld()->FindTeleportSpot(Pawn, Location, Rotation))
    {
        Location = GetActorLocation() + GetActorTransform().TransformVectorNoScale(ExitOffset);
        if (!GetWorld()->FindTeleportSpot(Pawn, Location, Rotation)) return;
    }
    Pawn->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
    if(Cabinet)Cabinet->SetDoorOpen(true);
    ReleaseOccupant();
}
void ABRHideSpot::ReleaseOccupant()
{
    if (!HasAuthority()) return;
    if (auto* Character = Cast<ABRPlayerCharacter>(Occupant)) Character->SetCurrentHideSpot(nullptr);
    Occupant = nullptr; ForceNetUpdate();
}
void ABRHideSpot::EndPlay(const EEndPlayReason::Type Reason)
{
    ReleaseOccupant(); Super::EndPlay(Reason);
}
void ABRHideSpot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ABRHideSpot, Occupant);
}
