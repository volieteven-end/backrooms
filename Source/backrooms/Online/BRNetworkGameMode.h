#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BRNetworkGameMode.generated.h"

/** Shared login/travel boundary for the lobby and parking map. */
UCLASS()
class BACKROOMS_API ABRNetworkGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ABRNetworkGameMode();
    virtual void StartPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual APlayerController* Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options,
        const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage) override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;
    virtual void HandleSeamlessTravelPlayer(AController*& Controller) override;
};

UCLASS()
class BACKROOMS_API ABRLobbyGameMode : public ABRNetworkGameMode
{
    GENERATED_BODY()
public:
    ABRLobbyGameMode();
};

UCLASS()
class BACKROOMS_API ABRMenuGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ABRMenuGameMode();
};
