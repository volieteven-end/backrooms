#pragma once
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "BRMenuPlayerController.generated.h"
class UBRMenuWidget;
class UBRInventoryWidget;
class UBRGameHUDWidget;
class UBRRoundResultWidget;
class ABRGarageDoor;

UCLASS()
class BACKROOMS_API ABRMenuPlayerController : public APlayerController
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void PlayerTick(float DeltaTime) override;
    virtual void SetupInputComponent() override;
    UFUNCTION(Server, Reliable, BlueprintCallable) void ServerSetLobbyReady(bool bReady);
    UFUNCTION(Server, Reliable, BlueprintCallable) void ServerStartRoom();
    UFUNCTION(Client, Reliable) void ClientMenuMessage(const FString& Message);
    UFUNCTION(Exec) void BRLeave();
    UFUNCTION(BlueprintCallable) void ToggleInventory();
    UFUNCTION(BlueprintCallable) void UseHeldItem();
    bool IsInventoryOpen() const {return Inventory!=nullptr;}
    bool IsResultScreenVisible() const {return RoundResult!=nullptr;}
private:
    void UpdatePresentation();
    void UpdateRoundResult();
    void CloseInventory();
    void QuickSlot(int32 Index);

    void DropHeldItem();
    UPROPERTY(Transient) TObjectPtr<UBRInventoryWidget> Inventory;
    void RunSmokeDriver();
    UPROPERTY(Transient) TObjectPtr<UBRMenuWidget> Menu;
    UPROPERTY(Transient) TObjectPtr<UBRGameHUDWidget> GameHUD;
    UPROPERTY(Transient) TObjectPtr<UBRRoundResultWidget> RoundResult;
    bool bRoundInputLocked=false;
    FString PresentedWorld, TestRole;
    double NextUpdate = 0, TestStarted = 0;
    bool bScreenshotRequested = false;
    TSet<FName> ItemSmokeObserved;
    TWeakObjectPtr<ABRGarageDoor> ItemSmokeDoor;
};
