#include "Online/BRRoomTypes.h"

bool FBRRoomModel::ValidName(const FString& Value, const int32 MaxLength)
{
    if (Value.IsEmpty() || Value.Len() > MaxLength || Value != Value.TrimStartAndEnd()) return false;
    for (TCHAR C : Value) if (C < 32 || C == 127 || C == TEXT('<') || C == TEXT('>')) return false;
    return true;
}

FBRDirectoryReply FBRRoomModel::Failure(const FString& Code, const FString& Message)
{
    FBRDirectoryReply R; R.Code = Code; R.Message = Message; return R;
}

FBRDirectoryReply FBRRoomModel::Create(const FString& Name, const FString& Nickname, int32 Capacity, const FString& Build, double Now)
{
    Expire(Now);
    if (Info.Phase != EBRRoomPhase::Idle) return Failure(TEXT("ROOM_EXISTS"), TEXT("已有房间，请在加入游戏中查看。"));
    if (Build != Info.BuildId) return Failure(TEXT("VERSION"), TEXT("客户端与服务器版本不一致。"));
    if (!ValidName(Name, 32) || !ValidName(Nickname, 24) || Capacity < 1 || Capacity > 4)
        return Failure(TEXT("INPUT"), TEXT("请填写昵称、房间名称和 1–4 人上限。"));
    Info.RoomId = FGuid::NewGuid(); Info.Name = Name; Info.MaxPlayers = Capacity; Info.Phase = EBRRoomPhase::Lobby;
    FBRDirectoryReply R = Reserve(Info.RoomId, Nickname, Build, Now);
    if (const FReservation* Reservation = Reservations.Find(R.Ticket)) Info.OwnerId = Reservation->Member.Id;
    Refresh(); R.Room = Info; return R;
}

FBRDirectoryReply FBRRoomModel::Reserve(const FGuid& RoomId, const FString& Nickname, const FString& Build, double Now)
{
    Expire(Now);
    if (Build != Info.BuildId) return Failure(TEXT("VERSION"), TEXT("客户端与服务器版本不一致。"));
    if (!ValidName(Nickname, 24)) return Failure(TEXT("NICKNAME"), TEXT("昵称需要 1–24 个字符。"));
    if (!Info.RoomId.IsValid() || RoomId != Info.RoomId) return Failure(TEXT("ROOM_GONE"), TEXT("房间已关闭，请刷新列表。"));
    if (Info.Phase != EBRRoomPhase::Lobby) return Failure(TEXT("IN_PROGRESS"), TEXT("游戏已开始，请等待下一局。"));
    if (Members.Num() + Reservations.Num() >= Info.MaxPlayers) return Failure(TEXT("FULL"), TEXT("房间已满。"));
    FReservation Res; Res.Member.Id = FGuid::NewGuid(); Res.Member.Nickname = Nickname;
    Res.Member.Order = ++NextOrder; Res.Expires = Now + 30.0;
    const FString Ticket = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Reservations.Add(Ticket, Res); Refresh();
    FBRDirectoryReply R; R.bSuccess = true; R.Code = TEXT("OK"); R.Room = Info; R.Ticket = Ticket; return R;
}

bool FBRRoomModel::CheckTicket(const FString& Ticket, const FString& Build, double Now) const
{
    const FReservation* Res = Reservations.Find(Ticket);
    return Info.Phase == EBRRoomPhase::Lobby && Build == Info.BuildId && Res && Res->Expires > Now;
}

bool FBRRoomModel::Admit(const FString& Ticket, const FString& Build, double Now, FBRRoomMember& OutMember)
{
    if (!CheckTicket(Ticket, Build, Now)) return false;
    OutMember = Reservations.FindChecked(Ticket).Member;
    OutMember.bReady = OutMember.Id == Info.OwnerId;
    Reservations.Remove(Ticket); Members.Add(OutMember.Id, OutMember); Refresh(); return true;
}

bool FBRRoomModel::SetReady(const FGuid& Id, bool bReady)
{
    FBRRoomMember* Member = Members.Find(Id);
    if (!Member || Info.Phase != EBRRoomPhase::Lobby) return false;
    Member->bReady = Id == Info.OwnerId || bReady; return true;
}

bool FBRRoomModel::CanStart(const FGuid& Id) const
{
    if (Info.Phase != EBRRoomPhase::Lobby || Id != Info.OwnerId || !Members.Contains(Id) || !Reservations.IsEmpty()) return false;
    for (const auto& Pair : Members) if (!Pair.Value.bReady) return false;
    return true;
}

bool FBRRoomModel::Start(const FGuid& Id)
{
    if (!CanStart(Id)) return false;
    Info.Phase = EBRRoomPhase::Starting; Refresh(); return true;
}

void FBRRoomModel::ElectOwner()
{
    if (Members.Contains(Info.OwnerId)) return;
    for (const auto& Pair : Reservations) if (Pair.Value.Member.Id == Info.OwnerId) return;
    Info.OwnerId.Invalidate(); uint64 First = MAX_uint64;
    for (const auto& Pair : Members) if (Pair.Value.Order < First) { Info.OwnerId = Pair.Key; First = Pair.Value.Order; }
    if (!Info.OwnerId.IsValid()) for (const auto& Pair : Reservations)
        if (Pair.Value.Member.Order < First) { Info.OwnerId = Pair.Value.Member.Id; First = Pair.Value.Member.Order; }
    if (FBRRoomMember* Owner = Members.Find(Info.OwnerId)) Owner->bReady = true;
}

void FBRRoomModel::Remove(const FGuid& Id)
{
    Members.Remove(Id); ElectOwner();
    if (Members.IsEmpty() && Reservations.IsEmpty()) Reset(); else Refresh();
}

void FBRRoomModel::Expire(double Now)
{
    bool bRemoved = false;
    for (auto It = Reservations.CreateIterator(); It; ++It) if (It.Value().Expires <= Now) { It.RemoveCurrent(); bRemoved = true; }
    if (bRemoved) { ElectOwner(); if (Members.IsEmpty() && Reservations.IsEmpty()) Reset(); }
    Refresh();
}

void FBRRoomModel::ReturnToLobby()
{
    if (Members.IsEmpty()) { Reset(); return; }
    Reservations.Empty(); Info.Phase = EBRRoomPhase::Lobby;
    for (auto& Pair : Members) Pair.Value.bReady = Pair.Key == Info.OwnerId;
    Refresh();
}

void FBRRoomModel::Reset() { const FString Build = Info.BuildId; Info = FBRRoomInfo(); Info.BuildId = Build; Members.Empty(); Reservations.Empty(); NextOrder = 0; }
void FBRRoomModel::Refresh()
{
    Info.CurrentPlayers = Members.Num();
    Info.OpenSlots = FMath::Max(0, Info.MaxPlayers - Members.Num() - Reservations.Num());
    Info.bJoinable = Info.Phase == EBRRoomPhase::Lobby && Info.OpenSlots > 0;
}
