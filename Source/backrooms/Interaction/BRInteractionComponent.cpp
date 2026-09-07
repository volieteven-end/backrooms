#include "Interaction/BRInteractionComponent.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "World/BRHideSpot.h"
#include "World/BRGarageDoor.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRSupplyPickup.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/BRInteractable.h"
#include "EngineUtils.h"
#include "Engine/World.h"

UBRInteractionComponent::UBRInteractionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

AActor* UBRInteractionComponent::FindFocusedInteractable() const
{
	const APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
	{
		return nullptr;
	}

    if (const auto* Character = Cast<ABRPlayerCharacter>(OwnerPawn))
        if (Character->GetCurrentHideSpot()) return Character->GetCurrentHideSpot();

	FVector ViewLocation;
	FRotator ViewRotation;
	if (const APlayerController* PlayerController = Cast<APlayerController>(OwnerPawn->GetController()))
	{
		PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}
	else
	{
		OwnerPawn->GetActorEyesViewPoint(ViewLocation, ViewRotation);
	}

	const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceDistance;
    return TraceTarget(ViewLocation,TraceEnd,OwnerPawn);
}

void UBRInteractionComponent::TryInteract()
{
	if (AActor* TargetActor = FindFocusedInteractable())
	{
		ServerInteract(TargetActor);
	}
}

void UBRInteractionComponent::ServerInteract_Implementation(AActor* TargetActor)
{
	if (!IsTargetValidForInteraction(TargetActor))
	{
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || FVector::DistSquared(OwnerPawn->GetActorLocation(), TargetActor->GetActorLocation()) > FMath::Square(TraceDistance * 1.25f))
	{
		return;
	}

    const auto* Character = Cast<ABRPlayerCharacter>(OwnerPawn);
    if (Character && Character->GetDownedComponent()->IsDowned()) return;
    const bool bOwnHideExit = Character && Character->GetCurrentHideSpot() == TargetActor;
    if (Character && Character->GetCurrentHideSpot() && !bOwnHideExit) return;
    if (!bOwnHideExit)
    {
        FVector Eye; FRotator Rotation; OwnerPawn->GetActorEyesViewPoint(Eye, Rotation);
        if (OwnerPawn->GetController()) Rotation = OwnerPawn->GetController()->GetControlRotation();
        if (TraceTarget(Eye,Eye+Rotation.Vector()*TraceDistance,OwnerPawn)!=TargetActor) return;
    }
	if (IBRInteractable::Execute_CanInteract(TargetActor, OwnerPawn))
	{
		IBRInteractable::Execute_Interact(TargetActor, OwnerPawn);
	}
}

bool UBRInteractionComponent::IsTargetValidForInteraction(AActor* TargetActor) const
{
	return IsValid(TargetActor) && TargetActor->GetClass()->ImplementsInterface(UBRInteractable::StaticClass());
}
AActor* UBRInteractionComponent::ResolveHitTarget(AActor* HitActor) const
{
    if(IsTargetValidForInteraction(HitActor))return HitActor;
    for(TActorIterator<ABRGarageDoor> It(GetWorld());It;++It)if(It->OwnsPanel(HitActor))return *It;
    return nullptr;
}
AActor* UBRInteractionComponent::TraceTarget(const FVector& Start,const FVector& End,const APawn* Pawn) const
{
    if(!GetWorld())return nullptr;
    // Query the visible surface, not a desk/doorframe's convex hull across an empty opening.
    FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(BRInteractionTrace),true,Pawn);
    if(!GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params))return nullptr;
    AActor* First=ResolveHitTarget(Hit.GetActor());
    if(Cast<ABRHideSpot>(First))
    {
        // The hiding trigger is not a wall. Let the player aim at loot inside an open cabinet.
        Params.AddIgnoredActor(First);FHitResult Behind;
        if(GetWorld()->LineTraceSingleByChannel(Behind,Start,End,ECC_Visibility,Params))
            if(Cast<ABRGarageKeyPickup>(Behind.GetActor()) || Cast<ABRSupplyPickup>(Behind.GetActor()))return Behind.GetActor();
    }
    return First;
}
