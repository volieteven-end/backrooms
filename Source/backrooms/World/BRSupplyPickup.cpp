#include "World/BRSupplyPickup.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRDownedComponent.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
ABRSupplyPickup::ABRSupplyPickup()
{
    bReplicates=true; bAlwaysRelevant=true; SetReplicateMovement(true);
    PickupTarget=CreateDefaultSubobject<USphereComponent>(TEXT("PickupTarget"));SetRootComponent(PickupTarget);
    PickupTarget->SetSphereRadius(14);PickupTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupTarget->SetCollisionResponseToAllChannels(ECR_Ignore);PickupTarget->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SupplyMesh"));Mesh->SetupAttachment(PickupTarget);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
void ABRSupplyPickup::OnConstruction(const FTransform& Transform) {Super::OnConstruction(Transform);ApplyState();}
void ABRSupplyPickup::BeginPlay() {Super::BeginPlay();ApplyState();}
void ABRSupplyPickup::ApplyState()
{
    const bool Battery=SupplyType==EBRSupplyType::Battery;
    const bool Water=SupplyType==EBRSupplyType::AlmondWater;
    const TCHAR* Path=Water?TEXT("/Game/Gameplay/Items/AlmondWater/SM_AlmondWater.SM_AlmondWater"):Battery?TEXT("/Game/ReverseAsset/ParkingGarage/MiddleFloorPackage/Meshes/a840318e4f/SM_Battery_01/StaticMeshes/SM_Battery_01.SM_Battery_01"):
        TEXT("/Game/Gameplay/Items/Reference/Meshes/SM_Flashlight/SM_Flashlight/StaticMeshes/SM_Flashlight.SM_Flashlight");
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));Mesh->SetRelativeLocation(FVector(0,0,Water?7.f:0.f));Mesh->SetRelativeScale3D(FVector(Battery?0.18f:1.0f));
    Mesh->EmptyOverrideMaterials();
    SetActorHiddenInGame(bCollected || !bContainerVisible);SetActorEnableCollision(!bCollected && bContainerVisible);
    if(bCollected && !bObservedCollected && GetWorld())
        if(auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>())Audio->PlayEvent(EBRGameplaySound::KeyPickup,GetActorLocation());
    bObservedCollected=bCollected;
}
bool ABRSupplyPickup::CanInteract_Implementation(APawn* Pawn) const
{
    const auto* P=Cast<ABRPlayerCharacter>(Pawn);
    return P && !P->GetDownedComponent()->IsDowned() && !bCollected && bContainerVisible &&
        P->GetInventoryComponent()->CanAdd(SupplyType==EBRSupplyType::Flashlight?EBRInventoryItem::Flashlight:SupplyType==EBRSupplyType::Battery?EBRInventoryItem::Battery:EBRInventoryItem::AlmondWater);
}
FText ABRSupplyPickup::GetInteractionText_Implementation(APawn* Pawn) const
{
    if (bCollected || !bContainerVisible) return FText::GetEmpty();
    if(!CanInteract_Implementation(Pawn))return FText::FromString(SupplyType==EBRSupplyType::Flashlight && Cast<ABRPlayerCharacter>(Pawn) && Cast<ABRPlayerCharacter>(Pawn)->HasFlashlight()?TEXT("已持有手电筒"):TEXT("背包或电池容量已满"));
    return FText::FromString(SupplyType==EBRSupplyType::Flashlight?TEXT("拾取手电筒"):SupplyType==EBRSupplyType::Battery?TEXT("拾取电池组"):TEXT("拾取杏仁水"));
}
void ABRSupplyPickup::Interact_Implementation(APawn* Pawn)
{
    if (!HasAuthority() || !CanInteract_Implementation(Pawn)) return;
    auto* P=CastChecked<ABRPlayerCharacter>(Pawn);
    const bool Granted=SupplyType==EBRSupplyType::Flashlight?P->GrantFlashlight(StoredCharge):SupplyType==EBRSupplyType::Battery?P->GrantBattery():P->GrantAlmondWater();
    if (!Granted) return;
    bCollected=true;ApplyState();ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("BR_ITEM result=COLLECTED type=%d player=%s"),int32(SupplyType),*P->GetName());
}
void ABRSupplyPickup::SetContainerVisible(bool Visible) {if(HasAuthority()){bContainerVisible=Visible;ApplyState();ForceNetUpdate();}}
void ABRSupplyPickup::OnRep_State(){ApplyState();}
void ABRSupplyPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABRSupplyPickup,SupplyType);DOREPLIFETIME(ABRSupplyPickup,bCollected);DOREPLIFETIME(ABRSupplyPickup,bContainerVisible);}
