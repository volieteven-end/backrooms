#pragma once
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "BRRoomSettings.generated.h"

UCLASS(Config=Game, DefaultConfig)
class BACKROOMS_API UBRRoomSettings : public UObject
{
    GENERATED_BODY()
public:
    UPROPERTY(Config, EditAnywhere, Category="Network") FString ServerHost = TEXT("127.0.0.1");
    UPROPERTY(Config, EditAnywhere, Category="Network") int32 GamePort = 7777;
    UPROPERTY(Config, EditAnywhere, Category="Network") int32 BeaconPort = 15000;
    UPROPERTY(Config, EditAnywhere, Category="Network") FString BuildId = TEXT("backrooms-room-v8-skinstealer-ai");
    UPROPERTY(Config, EditAnywhere, Category="Maps") FString MenuMap = TEXT("/Game/UI/Menu/Maps/L_MainMenu");
    UPROPERTY(Config, EditAnywhere, Category="Maps") FString LobbyMap = TEXT("/Game/UI/Menu/Maps/L_Lobby");
    UPROPERTY(Config, EditAnywhere, Category="Maps") FString GameplayMap = TEXT("/Game/ReverseAsset/ParkingGarage/LightingStudy/L_MiddleFloor_Dark");
    FString GetServerHost() const;
    int32 GetGamePort() const;
    int32 GetBeaconPort() const;
};
