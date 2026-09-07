#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BRItemSmokeProbe.generated.h"
class ABRPlayerCharacter;
class ABRGarageDoor;
class ABRLootCabinet;
class ABRSupplyPickup;
UCLASS()
class ABRItemSmokeProbe : public AActor
{
    GENERATED_BODY()
public:
    ABRItemSmokeProbe();
    virtual void Tick(float DeltaTime) override;
private:
    void Check(bool OK,const TCHAR* Name);
    void Aim(FVector Point);
    void Stand(AActor* Target,FVector LocalOffset);
    void Capture(const TCHAR* Name);
    void DebugAim(AActor* Target);
    UPROPERTY() TObjectPtr<ABRPlayerCharacter> Player;
    UPROPERTY() TObjectPtr<ABRGarageDoor> Door;
    UPROPERTY() TObjectPtr<ABRLootCabinet> Cabinet;
    UPROPERTY() TObjectPtr<ABRSupplyPickup> Torch;
    UPROPERTY() TObjectPtr<AActor> Leaf;
    double Started=0;
    int32 Stage=0,Failures=0,Checks=0,KeysBefore=0;
    FTransform LeafClosed;
    TSet<FString> Captured;
    UPROPERTY() TArray<TObjectPtr<ABRLootCabinet>> AuditCabinets;
    int32 AuditIndex=0,AuditPhase=0;
    double NextAuditTime=0;
};
