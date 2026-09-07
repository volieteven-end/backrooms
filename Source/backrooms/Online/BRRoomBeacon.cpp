#include "Online/BRRoomBeacon.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Engine/GameInstance.h"

ABRRoomBeaconClient::ABRRoomBeaconClient()
{
    OnHostConnectionFailure().BindUObject(this, &ThisClass::HandleFailure);
    BeaconConnectionInitialTimeout = 8.0f;
    BeaconConnectionTimeout = 15.0f;
}
void ABRRoomBeaconClient::OnConnected()
{
    Super::OnConnected();
    if (auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()) D->BeaconConnected(this);
}
void ABRRoomBeaconClient::HandleFailure()
{
    if (GetGameInstance()) if (auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()) D->BeaconFailed(this);

}
void ABRRoomBeaconClient::ServerRequest_Implementation(int32 RequestId, EBRDirectoryOperation Operation, const FGuid& RoomId,
    const FString& Name, const FString& Nickname, int32 Capacity, const FString& BuildId)
{
    const double Now = FPlatformTime::Seconds();
    if (Now - LastRequest < 0.25)
    {
        ClientReply(RequestId, FBRRoomModel::Failure(TEXT("RATE_LIMIT"), TEXT("操作太快，请稍后重试。"))); return;
    }
    LastRequest = Now;
    if (Name.Len() > 32 || Nickname.Len() > 24 || BuildId.Len() > 64)
    {
        ClientReply(RequestId, FBRRoomModel::Failure(TEXT("INPUT"), TEXT("输入内容过长。"))); return;
    }
    if (auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>())
        ClientReply(RequestId, D->HandleDirectoryRequest(Operation, RoomId, Name, Nickname, Capacity, BuildId));
}
void ABRRoomBeaconClient::ClientReply_Implementation(int32 RequestId, const FBRDirectoryReply& Reply)
{
    if (auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()) D->ReceiveReply(this, RequestId, Reply);
}
ABRRoomBeaconHostObject::ABRRoomBeaconHostObject()
{
    ClientBeaconActorClass = ABRRoomBeaconClient::StaticClass();
    BeaconTypeName = ClientBeaconActorClass->GetName();
}
AOnlineBeaconClient* ABRRoomBeaconHostObject::SpawnBeaconActor(UNetConnection* Connection)
{
    // This single-room directory needs only a small, bounded number of in-flight inquiries.
    return GetNumClientActors() < 32 ? Super::SpawnBeaconActor(Connection) : nullptr;
}
