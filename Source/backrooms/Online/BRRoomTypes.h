#pragma once

#include "CoreMinimal.h"
#include "BRRoomTypes.generated.h"

UENUM(BlueprintType)
enum class EBRRoomPhase : uint8 { Idle, Lobby, Starting, InGame, Ending };

UENUM(BlueprintType)
enum class EBRDirectoryOperation : uint8 { Query, Create, Join };

USTRUCT(BlueprintType)
struct BACKROOMS_API FBRRoomInfo
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FGuid RoomId;
    UPROPERTY(BlueprintReadOnly) FString Name;
    UPROPERTY(BlueprintReadOnly) FString BuildId = TEXT("backrooms-room-v1");
    UPROPERTY(BlueprintReadOnly) int32 CurrentPlayers = 0;
    UPROPERTY(BlueprintReadOnly) int32 MaxPlayers = 4;
    UPROPERTY(BlueprintReadOnly) int32 OpenSlots = 0;
    UPROPERTY(BlueprintReadOnly) EBRRoomPhase Phase = EBRRoomPhase::Idle;
    UPROPERTY(BlueprintReadOnly) FGuid OwnerId;
    UPROPERTY(BlueprintReadOnly) bool bJoinable = false;
    UPROPERTY(BlueprintReadOnly) int32 PingMs = 0;
};

USTRUCT(BlueprintType)
struct BACKROOMS_API FBRDirectoryReply
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) bool bSuccess = false;
    UPROPERTY(BlueprintReadOnly) FString Code;
    UPROPERTY(BlueprintReadOnly) FString Message;
    UPROPERTY(BlueprintReadOnly) FBRRoomInfo Room;
    // Never display or log this single-use credential.
    UPROPERTY() FString Ticket;
};

struct FBRRoomMember
{
    FGuid Id;
    FString Nickname;
    uint64 Order = 0;
    bool bReady = false;
};

/** Pure server state machine. Time is injected so races/expiry can be tested without a network. */
class BACKROOMS_API FBRRoomModel
{
public:
    FBRRoomInfo Info;
    TMap<FGuid, FBRRoomMember> Members;
    FBRDirectoryReply Create(const FString& Name, const FString& Nickname, int32 Capacity, const FString& Build, double Now);
    FBRDirectoryReply Reserve(const FGuid& RoomId, const FString& Nickname, const FString& Build, double Now);
    bool CheckTicket(const FString& Ticket, const FString& Build, double Now) const;
    bool Admit(const FString& Ticket, const FString& Build, double Now, FBRRoomMember& OutMember);
    bool SetReady(const FGuid& Id, bool bReady);
    bool Start(const FGuid& Id);
    bool CanStart(const FGuid& Id) const;
    void Remove(const FGuid& Id);
    void Expire(double Now);
    void ReturnToLobby();
    void Reset();
    void Refresh();
    static bool ValidName(const FString& Value, int32 MaxLength);
    static FBRDirectoryReply Failure(const FString& Code, const FString& Message);
private:
    struct FReservation { FBRRoomMember Member; double Expires = 0; };
    TMap<FString, FReservation> Reservations;
    uint64 NextOrder = 0;
    void ElectOwner();
};
