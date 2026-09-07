#include "Online/BRRoomSettings.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
FString UBRRoomSettings::GetServerHost() const
{
    FString Host = ServerHost;
    FParse::Value(FCommandLine::Get(), TEXT("BRServerHost="), Host);
    return Host;
}
int32 UBRRoomSettings::GetGamePort() const
{
    int32 Port = GamePort;
    FParse::Value(FCommandLine::Get(), TEXT("BRGamePort="), Port);
    return FMath::Clamp(Port, 1024, 65535);
}
int32 UBRRoomSettings::GetBeaconPort() const
{
    int32 Port = BeaconPort;
    FParse::Value(FCommandLine::Get(), TEXT("BRBeaconPort="), Port);
    return FMath::Clamp(Port, 1024, 65535);
}
