#pragma once
#include "CoreMinimal.h"
#include "OnlineBeaconClient.h"
#include "OnlineBeaconHostObject.h"
#include "Online/BRRoomTypes.h"
#include "BRRoomBeacon.generated.h"

UCLASS(Transient, NotPlaceable)
class BACKROOMS_API ABRRoomBeaconClient : public AOnlineBeaconClient
{
    GENERATED_BODY()
public:
    ABRRoomBeaconClient();
    virtual void OnConnected() override;
    void HandleFailure();
    UFUNCTION(Server, Reliable) void ServerRequest(int32 RequestId, EBRDirectoryOperation Operation, const FGuid& RoomId,
        const FString& Name, const FString& Nickname, int32 Capacity, const FString& BuildId);
    UFUNCTION(Client, Reliable) void ClientReply(int32 RequestId, const FBRDirectoryReply& Reply);
private:
    double LastRequest = -100.0;
};

UCLASS(Transient, NotPlaceable)
class BACKROOMS_API ABRRoomBeaconHostObject : public AOnlineBeaconHostObject
{
    GENERATED_BODY()
public:
    ABRRoomBeaconHostObject();
    virtual AOnlineBeaconClient* SpawnBeaconActor(UNetConnection* Connection) override;
};
