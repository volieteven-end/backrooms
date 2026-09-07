#pragma once
#include "CoreMinimal.h"
#include "World/BRGarageDoor.h"
#include "BRLootCabinet.generated.h"
class ABRHideSpot;
class ABRGarageKeyPickup;
class ABRSupplyPickup;
UCLASS()
class BACKROOMS_API ABRLootCabinet : public ABRGarageDoor
{
    GENERATED_BODY()
public:
    ABRLootCabinet();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual bool CanInteract_Implementation(APawn* Pawn) const override;
    virtual FText GetInteractionText_Implementation(APawn* Pawn) const override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void ConfigureLoot(int32 Type); // 0 empty, 1 objective key, 2 battery, 3 flashlight, 4 almond water; server only.
    ABRGarageKeyPickup* GetKey() const {return Key;}
    int32 GetLootType() const {return LootType;}
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Loot") FVector LootOffset=FVector(36,0,75);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Loot") bool bAllowObjectiveKey=true;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Loot",meta=(ClampMin="0",ClampMax="1")) float BatteryChance=0.35f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Loot",meta=(ClampMin="0",ClampMax="1")) float FlashlightChance=0.10f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Loot",meta=(ClampMin="0",ClampMax="1")) float AlmondWaterChance=0.25f;
    UPROPERTY(EditInstanceOnly,BlueprintReadWrite,Category="Backrooms|Loot") FName RequiredDoorTag;
    UPROPERTY(EditInstanceOnly,BlueprintReadWrite,Category="Backrooms|Loot") TObjectPtr<ABRHideSpot> HideSpot;
private:
    UPROPERTY(Replicated) int32 LootType=-1;
    UPROPERTY(Replicated) TObjectPtr<ABRGarageKeyPickup> Key;
    UPROPERTY(Replicated) TObjectPtr<ABRSupplyPickup> Supply;
    bool bLootVisible=false;
};
