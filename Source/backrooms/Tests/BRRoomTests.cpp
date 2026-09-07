#if WITH_DEV_AUTOMATION_TESTS
#include "Online/BRRoomTypes.h"
#include "Player/BRPlayerCharacter.h"
#include "World/BRHideSpot.h"
#include "Misc/AutomationTest.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRRoomLifecycleTest, "Backrooms.Rooms.Lifecycle", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBRRoomLifecycleTest::RunTest(const FString& Parameters)
{
    FBRRoomModel M; const FString Build = M.Info.BuildId; FBRRoomMember Host, Guest;
    TestFalse(TEXT("Invalid capacity"), M.Create(TEXT("Room"),TEXT("Host"),5,Build,0).bSuccess);
    TestFalse(TEXT("Wrong build"), M.Create(TEXT("Room"),TEXT("Host"),4,TEXT("old"),0).bSuccess);
    auto R = M.Create(TEXT("Room"),TEXT("Host"),2,Build,0); TestTrue(TEXT("Create"),R.bSuccess);
    TestFalse(TEXT("Second room rejected"),M.Create(TEXT("Other"),TEXT("Other"),2,Build,0).bSuccess);
    TestTrue(TEXT("Host login"),M.Admit(R.Ticket,Build,1,Host));
    TestFalse(TEXT("Ticket single use"),M.Admit(R.Ticket,Build,1,Guest));
    TestTrue(TEXT("Owner auto ready"),M.Members[Host.Id].bReady);
    auto G = M.Reserve(M.Info.RoomId,TEXT("Guest"),Build,2); TestTrue(TEXT("Last slot reserved"),G.bSuccess);
    TestFalse(TEXT("Concurrent slot race rejected"),M.Reserve(M.Info.RoomId,TEXT("Third"),Build,2).bSuccess);
    TestFalse(TEXT("Start blocked during connecting reservation"),M.Start(Host.Id));
    TestTrue(TEXT("Guest login"),M.Admit(G.Ticket,Build,3,Guest));
    TestFalse(TEXT("Owner waits for ready"),M.Start(Host.Id));
    TestFalse(TEXT("Guest cannot start"),M.Start(Guest.Id));
    TestTrue(TEXT("Ready"),M.SetReady(Guest.Id,true)); TestTrue(TEXT("Owner starts"),M.Start(Host.Id));
    TestFalse(TEXT("Duplicate start"),M.Start(Host.Id));
    TestFalse(TEXT("Late join rejected"),M.Reserve(M.Info.RoomId,TEXT("Late"),Build,4).bSuccess);
    TestFalse(TEXT("Ready rejected after start"),M.SetReady(Guest.Id,false));
    M.Info.Phase = EBRRoomPhase::InGame; M.Remove(Host.Id);
    TestEqual(TEXT("Host departure transfers owner"),M.Info.OwnerId,Guest.Id);
    TestEqual(TEXT("Server continues"),M.Info.Phase,EBRRoomPhase::InGame);
    M.Info.Phase = EBRRoomPhase::Ending; M.ReturnToLobby();
    TestEqual(TEXT("Round trip lobby"),M.Info.Phase,EBRRoomPhase::Lobby);
    TestTrue(TEXT("Solo owner can restart"),M.CanStart(Guest.Id));
    M.Remove(Guest.Id); TestEqual(TEXT("Empty room reset"),M.Info.Phase,EBRRoomPhase::Idle);
    TestFalse(TEXT("Old room identifier cleared"),M.Info.RoomId.IsValid());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRRoomExpiryTest, "Backrooms.Rooms.ExpiryAndInput", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBRRoomExpiryTest::RunTest(const FString& Parameters)
{
    FBRRoomModel M; const FString Build=M.Info.BuildId; FBRRoomMember Player;
    TestFalse(TEXT("Control characters"),FBRRoomModel::ValidName(TEXT("bad\nname"),24));
    TestTrue(TEXT("Chinese nickname"),FBRRoomModel::ValidName(TEXT("停车场玩家"),24));
    auto R=M.Create(TEXT("Room"),TEXT("Host"),4,Build,0);
    TestFalse(TEXT("Expired ticket"),M.CheckTicket(R.Ticket,Build,30));
    M.Expire(30); TestEqual(TEXT("Unclaimed creation releases room"),M.Info.Phase,EBRRoomPhase::Idle);
    auto New=M.Create(TEXT("New"),TEXT("Host"),4,Build,40);
    auto G=M.Reserve(M.Info.RoomId,TEXT("Guest"),Build,41);
    TestTrue(TEXT("Guest can connect before owner"),M.Admit(G.Ticket,Build,42,Player));
    M.Expire(70); TestEqual(TEXT("Expired creator promotes guest"),M.Info.OwnerId,Player.Id);
    TestFalse(TEXT("Old ticket cannot enter new room"),M.Admit(R.Ticket,Build,71,Player));
    TestTrue(TEXT("Released capacity available"),M.Reserve(M.Info.RoomId,TEXT("Next"),Build,71).bSuccess);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRRoomReplicationContractTest, "Backrooms.Rooms.ReplicationContracts", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBRRoomReplicationContractTest::RunTest(const FString& Parameters)
{
    const FProperty* Hidden = FindFProperty<FProperty>(ABRPlayerCharacter::StaticClass(),TEXT("CurrentHideSpot"));
    const FProperty* Occupant = FindFProperty<FProperty>(ABRHideSpot::StaticClass(),TEXT("Occupant"));
    TestTrue(TEXT("Hide state replicates with initial-state callback"),Hidden && Hidden->HasAllPropertyFlags(CPF_Net|CPF_RepNotify));
    TestTrue(TEXT("Hide occupant replicates"),Occupant && Occupant->HasAllPropertyFlags(CPF_Net));
    return true;
}
#endif
