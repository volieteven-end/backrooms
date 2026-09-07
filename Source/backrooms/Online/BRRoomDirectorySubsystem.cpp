#include "Online/BRRoomDirectorySubsystem.h"
#include "Online/BRRoomBeacon.h"
#include "Online/BRRoomSettings.h"
#include "OnlineBeaconHost.h"
#include "Core/BRGameState.h"
#include "Player/BRPlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/PackageName.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

void UBRRoomDirectorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection); bInitialized = true; Model.Info.BuildId = GetDefault<UBRRoomSettings>()->BuildId;
    NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &ThisClass::OnNetworkFailure);
    TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &ThisClass::OnTravelFailure);
}
void UBRRoomDirectorySubsystem::Deinitialize()
{
    bInitialized = false; CleanupClient(); StopHost();
    if (GEngine) { GEngine->OnNetworkFailure().Remove(NetworkFailureHandle); GEngine->OnTravelFailure().Remove(TravelFailureHandle); }
    Super::Deinitialize();
}
void UBRRoomDirectorySubsystem::QueryRooms() { if (!bBusy) BeginRequest(EBRDirectoryOperation::Query); }
void UBRRoomDirectorySubsystem::CreateRoom(const FString& RoomName, const FString& Nickname, int32 Capacity)
{
    if (bBusy) return;
    PendingName = RoomName.TrimStartAndEnd(); PendingNickname = Nickname.TrimStartAndEnd(); PendingCapacity = Capacity;
    BeginRequest(EBRDirectoryOperation::Create);
}
void UBRRoomDirectorySubsystem::JoinRoom(const FGuid& RoomId, const FString& Nickname)
{
    if (bBusy) return;
    PendingRoom = RoomId; PendingNickname = Nickname.TrimStartAndEnd(); BeginRequest(EBRDirectoryOperation::Join);
}
void UBRRoomDirectorySubsystem::CleanupClient()
{
    ABRRoomBeaconClient* Previous = ClientBeacon; ClientBeacon = nullptr;
    if (IsValid(Previous)) Previous->DestroyBeacon();
    bCleanupClient = false;
}
void UBRRoomDirectorySubsystem::BeginRequest(EBRDirectoryOperation Op)
{
    if (!GetWorld() || GetWorld()->GetNetMode() == NM_DedicatedServer) return;
    CleanupClient(); PendingOperation = Op; ++RequestId; bBusy = true; RequestStarted = FPlatformTime::Seconds();
    PendingTicket.Empty(); bConnectingGame = false;
    ClientBeacon = GetWorld()->SpawnActor<ABRRoomBeaconClient>();
    const UBRRoomSettings* Settings = GetDefault<UBRRoomSettings>();
    FURL URL(nullptr, *FString::Printf(TEXT("%s:%d"), *Settings->GetServerHost(), Settings->GetBeaconPort()), TRAVEL_Absolute);
    if (!ClientBeacon || !ClientBeacon->InitClient(URL)) FailClient(TEXT("CONNECT"), TEXT("服务器未连接，或正在切换关卡。请稍后刷新。"));
}
void UBRRoomDirectorySubsystem::BeaconConnected(ABRRoomBeaconClient* Client)
{
    if (Client != ClientBeacon || !bBusy) return;
    Client->ServerRequest(RequestId, PendingOperation, PendingRoom, PendingName, PendingNickname, PendingCapacity, GetDefault<UBRRoomSettings>()->BuildId);
}
void UBRRoomDirectorySubsystem::BeaconFailed(ABRRoomBeaconClient* Client)
{
    if (Client == ClientBeacon && bBusy && !bConnectingGame) FailClient(TEXT("OFFLINE"), TEXT("连接中断或服务器正在切图，请稍后刷新。"));
}
void UBRRoomDirectorySubsystem::ReceiveReply(ABRRoomBeaconClient* Client, int32 Id, const FBRDirectoryReply& Reply)
{
    if (Client != ClientBeacon || Id != RequestId || !bBusy) return;
    LastReply = Reply; LastReply.Room.PingMs = FMath::RoundToInt((FPlatformTime::Seconds() - RequestStarted) * 1000.0);
    bCleanupClient = true;
    if (Reply.bSuccess && PendingOperation != EBRDirectoryOperation::Query)
    { PendingTicket = Reply.Ticket; bConnectingGame = true; RequestStarted = FPlatformTime::Seconds(); LastReply.Message = TEXT("正在进入大厅…"); }
    else bBusy = false;
    LastReply.Ticket.Empty();
    UE_LOG(LogTemp, Display, TEXT("BR_DIRECTORY op=%d success=%d code=%s players=%d"), int32(PendingOperation), Reply.bSuccess, *Reply.Code, Reply.Room.CurrentPlayers);
    OnDirectoryChanged.Broadcast(LastReply);
}
void UBRRoomDirectorySubsystem::FailClient(const FString& Code, const FString& Message)
{
    bBusy = false; bCleanupClient = true; PendingTicket.Empty(); bConnectingGame = false;
    // Preserve the last room snapshot: an interrupted query is not an empty successful list.
    LastReply.bSuccess = false; LastReply.Code = Code; LastReply.Message = Message; LastReply.Ticket.Empty();
    OnDirectoryChanged.Broadcast(LastReply);
}
void UBRRoomDirectorySubsystem::LeaveRoom()
{
    CleanupClient(); bBusy = false; bConnectingGame = false; PendingTicket.Empty(); ++RequestId;
    LastReply.Message.Empty(); LastReply.Ticket.Empty();
    bReturningHome = true;
}
void UBRRoomDirectorySubsystem::OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error)
{
    if (World != GetWorld() || World->GetNetMode() == NM_DedicatedServer) return;
    // Beacon failures already have a scoped handler and must not disconnect gameplay.
    if (Driver && Driver->NetDriverName.ToString().Contains(TEXT("Beacon"))) return;
    FailClient(TEXT("NETWORK"), TEXT("游戏连接中断，已返回主菜单。")); bReturningHome = true;
}
void UBRRoomDirectorySubsystem::OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error)
{
    if (World != GetWorld()) return;
    if (World->GetNetMode() == NM_DedicatedServer)
    {
        Model.ReturnToLobby(); TravelStarted = 0; ServerWorldReady(World);
        UE_LOG(LogTemp, Error, TEXT("BR_SERVER_TRAVEL result=FAIL type=%d"), int32(Type));
    }
    else { FailClient(TEXT("TRAVEL"), TEXT("关卡加载失败，已返回主菜单。")); bReturningHome = true; }
}
FBRDirectoryReply UBRRoomDirectorySubsystem::HandleDirectoryRequest(EBRDirectoryOperation Op, const FGuid& RoomId,
    const FString& Name, const FString& Nickname, int32 Capacity, const FString& Build)
{
    if (!GetWorld() || GetWorld()->GetNetMode() != NM_DedicatedServer)
        return FBRRoomModel::Failure(TEXT("NOT_SERVER"), TEXT("此进程不是独立服务器。"));
    Model.Expire(FPlatformTime::Seconds());
    if (Build != Model.Info.BuildId) return FBRRoomModel::Failure(TEXT("VERSION"), TEXT("客户端与服务器版本不一致。"));
    if (Op == EBRDirectoryOperation::Create && !GetWorld()->GetMapName().EndsWith(TEXT("L_Lobby")))
        return FBRRoomModel::Failure(TEXT("RESETTING"), TEXT("服务器正在返回大厅，请稍后刷新。"));
    FBRDirectoryReply R;
    switch (Op)
    {
    case EBRDirectoryOperation::Query: R.bSuccess = true; R.Code = TEXT("OK"); R.Room = Model.Info; break;
    case EBRDirectoryOperation::Create: R = Model.Create(Name, Nickname, Capacity, Build, FPlatformTime::Seconds()); break;
    case EBRDirectoryOperation::Join: R = Model.Reserve(RoomId, Nickname, Build, FPlatformTime::Seconds()); break;
    default: R = FBRRoomModel::Failure(TEXT("OPERATION"), TEXT("未知操作。")); break;
    }
    PublishServerState(); return R;
}
void UBRRoomDirectorySubsystem::StopHost()
{
    if (IsValid(HostObject)) { HostObject->Unregister(); HostObject->Destroy(); }
    HostObject = nullptr;
    if (IsValid(Host)) Host->DestroyBeacon();
    Host = nullptr;
}
void UBRRoomDirectorySubsystem::ServerWorldLeaving(UWorld* World)
{
    if (ServerWorld.Get() == World) { StopHost(); ServerWorld.Reset(); }
}
void UBRRoomDirectorySubsystem::ServerWorldReady(UWorld* World)
{
    if (!World || World->GetNetMode() != NM_DedicatedServer) return;
    StopHost(); ServerWorld = World; TravelStarted = 0;
    const bool bLobby = World->GetMapName().EndsWith(TEXT("L_Lobby"));
    if (!bLobby && Model.Info.Phase == EBRRoomPhase::Starting) { Model.Info.Phase = EBRRoomPhase::InGame; SmokeRoundStarted = FPlatformTime::Seconds(); }
    Host = World->SpawnActor<AOnlineBeaconHost>();
    Host->ListenPort = GetDefault<UBRRoomSettings>()->GetBeaconPort();
    if (Host->InitHost())
    {
        HostObject = World->SpawnActor<ABRRoomBeaconHostObject>(); Host->RegisterHost(HostObject); Host->PauseBeaconRequests(false);
        UE_LOG(LogTemp, Display, TEXT("BR_BEACON result=LISTENING port=%d map=%s"), Host->ListenPort, *World->GetMapName());
    }
    else { StopHost(); UE_LOG(LogTemp, Error, TEXT("BR_BEACON result=BIND_FAILED")); }
    Model.Refresh(); PublishServerState();
}
void UBRRoomDirectorySubsystem::ApplyMember(ABRPlayerState* State)
{
    if (!State || !State->HasAuthority()) return;
    if (const FBRRoomMember* M = Model.Members.Find(State->GetRoomPlayerId()))
        State->SetRoomIdentity(M->Id, M->Nickname, M->Id == Model.Info.OwnerId, M->bReady);
}
void UBRRoomDirectorySubsystem::PublishServerState()
{
    Model.Refresh();
    if (GetWorld()) if (ABRGameState* GS = GetWorld()->GetGameState<ABRGameState>())
    {
        GS->SetRoomInfo(Model.Info);
        for (APlayerState* PS : GS->PlayerArray) ApplyMember(Cast<ABRPlayerState>(PS));
    }
}
bool UBRRoomDirectorySubsystem::RequestReady(ABRPlayerState* State, bool bReady)
{
    const bool bOK = State && Model.SetReady(State->GetRoomPlayerId(), bReady); PublishServerState(); return bOK;
}
bool UBRRoomDirectorySubsystem::TravelServer(const FString& Map)
{
    if (!GetWorld() || !FPackageName::DoesPackageExist(Map)) return false;
    StopHost(); TravelStarted = FPlatformTime::Seconds();
    const bool bOK = GetWorld()->ServerTravel(Map, true);
    if (!bOK) { TravelStarted = 0; ServerWorldReady(GetWorld()); }
    return bOK;
}
bool UBRRoomDirectorySubsystem::RequestStart(ABRPlayerState* State)
{
    if (!State || !Model.Start(State->GetRoomPlayerId())) return false;
    PublishServerState();
    if (!TravelServer(GetDefault<UBRRoomSettings>()->GameplayMap)) { Model.ReturnToLobby(); PublishServerState(); return false; }
    UE_LOG(LogTemp, Display, TEXT("BR_ROOM result=START players=%d"), Model.Members.Num()); return true;
}
void UBRRoomDirectorySubsystem::FinishRound(bool bWon)
{
    if (Model.Info.Phase != EBRRoomPhase::InGame) return;
    Model.Info.Phase = EBRRoomPhase::Ending; RoundEnds = FPlatformTime::Seconds() + 5.0;
    if (auto* GS = GetWorld()->GetGameState<ABRGameState>()) GS->SetLevelPhase(bWon ? EBRLevelPhase::Completed : EBRLevelPhase::Failed);
    PublishServerState(); UE_LOG(LogTemp, Display, TEXT("BR_ROOM result=ROUND_END won=%d"), bWon);
}
void UBRRoomDirectorySubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld(); if (!World || !World->IsGameWorld()) return;
    const double Now = FPlatformTime::Seconds();
    if (World->GetNetMode() == NM_DedicatedServer)
    {
        if (Now < NextServerPoll) return; NextServerPoll = Now + 0.5;
        Model.Expire(Now);
        if (TravelStarted > 0) return;
        if (Model.Info.Phase == EBRRoomPhase::InGame)
        {
            int32 Active = 0, Downed = 0;
            if (auto* GS = World->GetGameState<ABRGameState>()) for (APlayerState* Base : GS->PlayerArray)
                if (auto* PS = Cast<ABRPlayerState>(Base)) if (Model.Members.Contains(PS->GetRoomPlayerId()) && PS->GetPawn())
                { ++Active; if (PS->IsDowned()) ++Downed; }
            if (Active > 0 && Active == Downed) FinishRound(false);
#if !UE_BUILD_SHIPPING
            // Explicit local test launch only. This tests round-trip travel, not actual extraction success.
            if (FParse::Param(FCommandLine::Get(), TEXT("BRNetworkSmoke")) && !FParse::Param(FCommandLine::Get(),TEXT("BRRealRoundSmoke")) && Active >= 2 && Now - SmokeRoundStarted > 15.0) FinishRound(true);
#endif
        }
        if ((Model.Info.Phase == EBRRoomPhase::Ending && Now >= RoundEnds) ||
            (Model.Info.Phase == EBRRoomPhase::Idle && !World->GetMapName().EndsWith(TEXT("L_Lobby"))))
        {
            Model.ReturnToLobby(); PublishServerState(); TravelServer(GetDefault<UBRRoomSettings>()->LobbyMap); return;
        }
        PublishServerState(); return;
    }
    if (bCleanupClient) CleanupClient();
    if (bReturningHome)
    {
        bReturningHome = false;
        UGameplayStatics::OpenLevel(World, FName(*GetDefault<UBRRoomSettings>()->MenuMap), true); return;
    }
    if (!PendingTicket.IsEmpty())
    {
        if (APlayerController* PC = GetGameInstance()->GetFirstLocalPlayerController())
        {
            const auto* S = GetDefault<UBRRoomSettings>();
            const FString URL = FString::Printf(TEXT("%s:%d?BRBuild=%s?BRTicket=%s"), *S->GetServerHost(), S->GetGamePort(), *S->BuildId, *PendingTicket);
            PendingTicket.Empty(); PC->ClientTravel(URL, TRAVEL_Absolute);
        }
    }
    if (bConnectingGame && World->GetNetMode() == NM_Client && World->GetMapName().EndsWith(TEXT("L_Lobby")))
    { bConnectingGame = false; bBusy = false; LastReply.Message.Empty(); }
    if (bBusy && Now - RequestStarted > (bConnectingGame ? 60.0 : 10.0))
    { const bool bWasConnecting = bConnectingGame; FailClient(TEXT("TIMEOUT"), TEXT("连接超时，请检查服务器地址并重试。")); bReturningHome |= bWasConnecting; }
}
