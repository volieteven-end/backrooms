#include "Online/BRNetworkGameMode.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Player/BRPlayerState.h"
#include "Player/BRPlayerCharacter.h"
#include "World/BRHideSpot.h"
#include "Core/BRGameState.h"
#include "UI/BRMenuPlayerController.h"
#include "Engine/GameInstance.h"
#include "Kismet/GameplayStatics.h"

ABRNetworkGameMode::ABRNetworkGameMode()
{
    bUseSeamlessTravel = true;
    PlayerControllerClass = ABRMenuPlayerController::StaticClass();
    PlayerStateClass = ABRPlayerState::StaticClass(); GameStateClass = ABRGameState::StaticClass();
}
void ABRNetworkGameMode::StartPlay()
{
    Super::StartPlay();
    GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->ServerWorldReady(GetWorld());
}
void ABRNetworkGameMode::EndPlay(const EEndPlayReason::Type Reason)
{
    if (GetGameInstance()) GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->ServerWorldLeaving(GetWorld());
    Super::EndPlay(Reason);
}
void ABRNetworkGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
    if (!ErrorMessage.IsEmpty() || GetNetMode() != NM_DedicatedServer) return;
    const auto& Model = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->ServerModel();
    if (!Model.CheckTicket(UGameplayStatics::ParseOption(Options, TEXT("BRTicket")), UGameplayStatics::ParseOption(Options, TEXT("BRBuild")), FPlatformTime::Seconds()))
        ErrorMessage = TEXT("ROOM_TICKET_EXPIRED_OR_INVALID");
}
APlayerController* ABRNetworkGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal,
    const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
    APlayerController* PC = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
    if (!PC || !ErrorMessage.IsEmpty() || GetNetMode() != NM_DedicatedServer) return PC;
    auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>(); FBRRoomMember Member;
    ABRPlayerState* PS = PC->GetPlayerState<ABRPlayerState>();
    if (!PS || !D->ServerModel().Admit(UGameplayStatics::ParseOption(Options, TEXT("BRTicket")),
        UGameplayStatics::ParseOption(Options, TEXT("BRBuild")), FPlatformTime::Seconds(), Member))
    { ErrorMessage = TEXT("ROOM_TICKET_EXPIRED_OR_INVALID"); PC->Destroy(); return nullptr; }
    PS->SetRoomIdentity(Member.Id, Member.Nickname, Member.Id == D->ServerModel().Info.OwnerId, Member.bReady);
    UE_LOG(LogTemp, Display, TEXT("BR_LOGIN result=ADMITTED players=%d"), D->ServerModel().Members.Num()); return PC;
}
void ABRNetworkGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer); GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->PublishServerState();
}
void ABRNetworkGameMode::Logout(AController* Exiting)
{
    auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>();
    if (auto* Character = Cast<ABRPlayerCharacter>(Exiting->GetPawn()))
        if (ABRHideSpot* Spot = Character->GetCurrentHideSpot()) Spot->ReleaseOccupant();
    if (auto* PS = Exiting->GetPlayerState<ABRPlayerState>()) D->ServerModel().Remove(PS->GetRoomPlayerId());
    Super::Logout(Exiting); D->PublishServerState();
    UE_LOG(LogTemp, Display, TEXT("BR_LOGOUT players=%d"), D->ServerModel().Members.Num());
}
void ABRNetworkGameMode::HandleSeamlessTravelPlayer(AController*& Controller)
{
    Super::HandleSeamlessTravelPlayer(Controller);
    if (Controller) if (auto* PS = Controller->GetPlayerState<ABRPlayerState>())
    {
        PS->SetDownedState(false);
        GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->ApplyMember(PS);
    }
}
ABRLobbyGameMode::ABRLobbyGameMode() { DefaultPawnClass = nullptr; bStartPlayersAsSpectators = true; }
ABRMenuGameMode::ABRMenuGameMode()
{
    DefaultPawnClass = nullptr; PlayerControllerClass = ABRMenuPlayerController::StaticClass();
    PlayerStateClass = ABRPlayerState::StaticClass(); GameStateClass = ABRGameState::StaticClass();
}
