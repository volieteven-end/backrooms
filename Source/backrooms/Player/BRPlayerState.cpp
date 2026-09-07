#include "Player/BRPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Engine/GameInstance.h"

ABRPlayerState::ABRPlayerState()
{
	SetNetUpdateFrequency(10.0f);
}

void ABRPlayerState::ServerSetReady_Implementation(const bool bNewReady)
{
	if (GetGameInstance()) GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->RequestReady(this, bNewReady);
}

void ABRPlayerState::SetDownedState(const bool bNewDowned)
{
	if (HasAuthority() && bDowned != bNewDowned)
	{
		bDowned = bNewDowned;
		OnPlayerStateChanged.Broadcast();
		ForceNetUpdate();
	}
}

void ABRPlayerState::OnRep_PlayerFlags()
{
	OnPlayerStateChanged.Broadcast();
}

void ABRPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABRPlayerState, RoomPlayerId);
	DOREPLIFETIME(ABRPlayerState, bRoomOwner);
	DOREPLIFETIME(ABRPlayerState, bReady);
	DOREPLIFETIME(ABRPlayerState, bDowned);
}

void ABRPlayerState::SetRoomIdentity(const FGuid& Id, const FString& Nickname, bool bOwner, bool bNewReady)
{
    if (!HasAuthority()) return;
    const bool bChanged = RoomPlayerId != Id || bRoomOwner != bOwner || bReady != bNewReady || GetPlayerName() != Nickname;
    RoomPlayerId = Id; bRoomOwner = bOwner; bReady = bNewReady;
    if (GetPlayerName() != Nickname) SetPlayerName(Nickname);
    if (bChanged) { ForceNetUpdate(); OnPlayerStateChanged.Broadcast(); }
}
void ABRPlayerState::CopyProperties(APlayerState* Target)
{
    Super::CopyProperties(Target);
    if (auto* PS = Cast<ABRPlayerState>(Target)) PS->SetRoomIdentity(RoomPlayerId, GetPlayerName(), bRoomOwner, bReady);
}
