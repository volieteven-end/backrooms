#include "World/BRLootCabinet.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRSupplyPickup.h"
#include "World/BRHideSpot.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
ABRLootCabinet::ABRLootCabinet(){bUnlockable=true;bOpenAwayFromPlayer=false;OpenAngle=105;}
void ABRLootCabinet::BeginPlay(){Super::BeginPlay();}
bool ABRLootCabinet::CanInteract_Implementation(APawn* Pawn) const
{return Super::CanInteract_Implementation(Pawn);}
FText ABRLootCabinet::GetInteractionText_Implementation(APawn*) const
{
    return IsOpen()?NSLOCTEXT("Backrooms","CloseCabinet","关闭柜门 / 抽屉"):NSLOCTEXT("Backrooms","OpenCabinet","打开柜门 / 抽屉搜寻");
}
void ABRLootCabinet::ConfigureLoot(int32 Type)
{
    if (!HasAuthority() || LootType!=-1)return;
    LootType=FMath::Clamp(Type,0,4);
    const FVector P=GetActorTransform().TransformPosition(LootOffset);
    if(LootType==1)
    {
        FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Key=GetWorld()->SpawnActor<ABRGarageKeyPickup>(P,FRotator::ZeroRotator,Params);
        if(Key){Key->SetActorScale3D(FVector(2));Key->SetContainerLocked(true);Key->SetPickupActive(true);}
    }
    else if(LootType>=2)
    {
        const FTransform Transform(GetActorRotation(),P);
        Supply=GetWorld()->SpawnActorDeferred<ABRSupplyPickup>(ABRSupplyPickup::StaticClass(),Transform);
        if(Supply){Supply->SupplyType=LootType==2?EBRSupplyType::Battery:LootType==4?EBRSupplyType::AlmondWater:EBRSupplyType::Flashlight;Supply->SetContainerVisible(false);Supply->FinishSpawning(Transform);}
    }
    ForceNetUpdate();
}
void ABRLootCabinet::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(!HasAuthority())return;
    const bool Visible=IsOpen() && GetOpenAlpha()>0.8f;
    if(Visible!=bLootVisible)
    {
        bLootVisible=Visible;
        if(Key)Key->SetContainerLocked(!Visible);
        if(Supply)Supply->SetContainerVisible(Visible);
    }
}
void ABRLootCabinet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABRLootCabinet,LootType);DOREPLIFETIME(ABRLootCabinet,Key);DOREPLIFETIME(ABRLootCabinet,Supply);}
