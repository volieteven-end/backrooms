#include "Player/BRDownedComponent.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/BRPlayerState.h"
#include "Net/UnrealNetwork.h"

UBRDownedComponent::UBRDownedComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UBRDownedComponent::Down()
{
	if (GetOwner() && GetOwner()->HasAuthority() && !bDowned)
	{
		bDowned = true;
		ApplyDownedState();
	}
}

void UBRDownedComponent::Revive(APawn* Reviver)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !bDowned || !IsValid(Reviver) || Reviver == GetOwner())
	{
		return;
	}

	if (FVector::DistSquared(Reviver->GetActorLocation(), GetOwner()->GetActorLocation()) <= FMath::Square(300.0f))
	{
		bDowned = false;
		ApplyDownedState();
	}
}

void UBRDownedComponent::OnRep_Downed()
{
	ApplyDownedState();
}

void UBRDownedComponent::ApplyDownedState()
{
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		if (bDowned)
		{
			Character->GetCharacterMovement()->DisableMovement();
		}
		else
		{
			Character->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}

		if (Character->HasAuthority())
		{
			if (ABRPlayerState* State = Character->GetPlayerState<ABRPlayerState>())
			{
				State->SetDownedState(bDowned);
			}
		}
	}

	OnDownedStateChanged.Broadcast(bDowned);
}

void UBRDownedComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBRDownedComponent, bDowned);
}
