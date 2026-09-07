#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "Engine/EngineBaseTypes.h"
#include "Online/BRRoomTypes.h"
#include "BRRoomDirectorySubsystem.generated.h"

class ABRRoomBeaconClient;
class AOnlineBeaconHost;
class ABRRoomBeaconHostObject;
class ABRPlayerState;
class UNetDriver;
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRDirectoryChanged, const FBRDirectoryReply&, Result);

UCLASS()
class BACKROOMS_API UBRRoomDirectorySubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual void Tick(float DeltaTime) override;
    virtual bool IsTickable() const override { return bInitialized && !IsTemplate(); }
    virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UBRRoomDirectorySubsystem, STATGROUP_Tickables); }
    virtual UWorld* GetTickableGameObjectWorld() const override { return GetWorld(); }
    UFUNCTION(BlueprintCallable) void QueryRooms();
    UFUNCTION(BlueprintCallable) void CreateRoom(const FString& RoomName, const FString& Nickname, int32 Capacity);
    UFUNCTION(BlueprintCallable) void JoinRoom(const FGuid& RoomId, const FString& Nickname);
    UFUNCTION(BlueprintCallable) void LeaveRoom();
    UPROPERTY(BlueprintReadOnly) bool bBusy = false;
    UPROPERTY(BlueprintReadOnly) FBRDirectoryReply LastReply;
    UPROPERTY(BlueprintAssignable) FBRDirectoryChanged OnDirectoryChanged;

    void BeaconConnected(ABRRoomBeaconClient* Client);
    void BeaconFailed(ABRRoomBeaconClient* Client);
    void ReceiveReply(ABRRoomBeaconClient* Client, int32 Id, const FBRDirectoryReply& Reply);
    FBRDirectoryReply HandleDirectoryRequest(EBRDirectoryOperation Op, const FGuid& RoomId, const FString& Name,
        const FString& Nickname, int32 Capacity, const FString& Build);
    void ServerWorldReady(UWorld* World);
    void ServerWorldLeaving(UWorld* World);
    void PublishServerState();
    void ApplyMember(ABRPlayerState* State);
    bool RequestReady(ABRPlayerState* State, bool bReady);
    bool RequestStart(ABRPlayerState* State);
    void FinishRound(bool bWon);
    bool bSmokeCreate=false, bSmokeJoin=false, bSmokeStart=false, bSmokeSawGame=false;
    double SmokeStarted=0, SmokeRoundStarted=0;
    bool bSmokeHiddenObserved=false, bSmokeExitObserved=false, bSmokeKeysObserved=false, bSmokeDoorObserved=false;
    bool bSmokeRemoteHidden=false, bSmokeRemoteExit=false;
    int32 SmokeRoundsCompleted=0;
    double SmokeLobbyEntered=0;
    bool bSmokeWasInLobby=false, bSmokeDeliberateLeave=false;
    bool bSmokeIllegalStart=false;
    bool bSmokeBrowseOpened=false;
    FBRRoomModel& ServerModel() { return Model; }
private:
    void BeginRequest(EBRDirectoryOperation Op);
    void CleanupClient();
    void StopHost();
    bool TravelServer(const FString& Map);
    void FailClient(const FString& Code, const FString& Message);
    void OnNetworkFailure(UWorld* World, UNetDriver* Driver, ENetworkFailure::Type Type, const FString& Error);
    void OnTravelFailure(UWorld* World, ETravelFailure::Type Type, const FString& Error);
    FBRRoomModel Model;
    UPROPERTY(Transient) TObjectPtr<ABRRoomBeaconClient> ClientBeacon;
    UPROPERTY(Transient) TObjectPtr<AOnlineBeaconHost> Host;
    UPROPERTY(Transient) TObjectPtr<ABRRoomBeaconHostObject> HostObject;
    TWeakObjectPtr<UWorld> ServerWorld;
    FDelegateHandle NetworkFailureHandle, TravelFailureHandle;
    EBRDirectoryOperation PendingOperation = EBRDirectoryOperation::Query;
    FGuid PendingRoom;
    FString PendingName, PendingNickname, PendingTicket;
    int32 PendingCapacity = 4, RequestId = 0;
    double RequestStarted = 0, RoundEnds = 0, NextServerPoll = 0, TravelStarted = 0;
    bool bInitialized = false, bCleanupClient = false, bConnectingGame = false, bReturningHome = false;
};
