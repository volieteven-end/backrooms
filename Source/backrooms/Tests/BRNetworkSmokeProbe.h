#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRNetworkSmokeProbe.generated.h"
class ABRPlayerCharacter;
class ABRHideSpot;
class ABREntityCharacter;
class ABRGarageDoor;
UCLASS(Transient,NotPlaceable)
class ABRNetworkSmokeProbe : public AActor
{
    GENERATED_BODY()
public:
    ABRNetworkSmokeProbe();
    virtual void Tick(float DeltaTime) override;
private:
    double Started = 0;
    int32 Stage = 0;
    TWeakObjectPtr<ABRPlayerCharacter> TestPlayer;
    TWeakObjectPtr<ABRHideSpot> Spot;
    TWeakObjectPtr<ABREntityCharacter> Entity;
    FVector EntityStart;
    TWeakObjectPtr<ABRGarageDoor> TestedDoor;
    bool bBatteryTested=false;
};
