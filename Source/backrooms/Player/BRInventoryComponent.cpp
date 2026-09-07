#include "Player/BRInventoryComponent.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "World/BRSupplyPickup.h"
#include "Core/BRGameState.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
UBRInventoryComponent::UBRInventoryComponent()
{SetIsReplicatedByDefault(true);PrimaryComponentTick.bCanEverTick=true;Slots.SetNum(Capacity);}
int32 UBRInventoryComponent::Count(EBRInventoryItem Item) const
{int32 N=0;for(const auto& S:Slots)if(S.Item==Item)++N;return N;}
int32 UBRInventoryComponent::GetUsedSlots() const {return Capacity-Count(EBRInventoryItem::Empty);}
bool UBRInventoryComponent::CanAdd(EBRInventoryItem Item) const
{return Item!=EBRInventoryItem::Empty && uint8(Item)<=uint8(EBRInventoryItem::AlmondWater) && Count(EBRInventoryItem::Empty)>0 &&
    (Item!=EBRInventoryItem::Flashlight || Count(Item)==0) && (Item!=EBRInventoryItem::Battery || Count(Item)<5);}
bool UBRInventoryComponent::CanOperate() const
{const auto* P=Cast<ABRPlayerCharacter>(GetOwner());return P && !P->GetDownedComponent()->IsDowned() && !P->GetCurrentHideSpot();}
bool UBRInventoryComponent::Matches(int32 I,FGuid Id) const
{return Slots.IsValidIndex(I) && Slots[I].Item!=EBRInventoryItem::Empty && Id.IsValid() && Slots[I].InstanceId==Id;}
bool UBRInventoryComponent::AddItem(EBRInventoryItem Item)
{
    if(!GetOwner()->HasAuthority() || !CanAdd(Item))return false;
    int32 Index=Slots[MainHandSlot].Item==EBRInventoryItem::Empty ? MainHandSlot : INDEX_NONE;
    if(Index==INDEX_NONE)for(int32 I=0;I<Capacity;++I)if(Slots[I].Item==EBRInventoryItem::Empty){Index=I;break;}
    if(Index!=INDEX_NONE){Slots[Index].Item=Item;Slots[Index].InstanceId=FGuid::NewGuid();Changed();return true;}
    return false;
}
void UBRInventoryComponent::Changed()
{EquippedItem=Slots[MainHandSlot].Item;SelectedSlot=EquippedItem==EBRInventoryItem::Empty?INDEX_NONE:MainHandSlot;
 OnRep_Inventory();if(auto* P=Cast<ABRPlayerCharacter>(GetOwner()))P->RefreshInventoryEquipment();GetOwner()->ForceNetUpdate();}
void UBRInventoryComponent::OnRep_Inventory(){OnInventoryChanged.Broadcast();}
void UBRInventoryComponent::RemoveAt(int32 I)
{Slots[I]=FBRInventorySlot();if(SelectedSlot==I){SelectedSlot=INDEX_NONE;EquippedItem=EBRInventoryItem::Empty;}Changed();}
bool UBRInventoryComponent::ConsumeBattery()
{
    if(!GetOwner()->HasAuthority())return false;
    for(int32 I=0;I<Capacity;++I)if(Slots[I].Item==EBRInventoryItem::Battery){RemoveAt(I);return true;}return false;
}
void UBRInventoryComponent::SelectFirst(EBRInventoryItem Item)
{if(GetOwner()->HasAuthority())for(int32 I=0;I<Capacity;++I)if(Slots[I].Item==Item){Swap(Slots[I],Slots[MainHandSlot]);Changed();break;}}
void UBRInventoryComponent::SelectSlot(int32 I)
{if(Slots.IsValidIndex(I))ServerSelectSlot(I,Slots[I].InstanceId);}
void UBRInventoryComponent::UseSelected(){if(Slots.IsValidIndex(SelectedSlot))ServerUseSlot(SelectedSlot,Slots[SelectedSlot].InstanceId);}
void UBRInventoryComponent::DropSelected(){if(Slots.IsValidIndex(SelectedSlot))ServerDropSlot(SelectedSlot,Slots[SelectedSlot].InstanceId);}
void UBRInventoryComponent::ServerSelectSlot_Implementation(int32 I,FGuid Id)
{if(Slots.IsValidIndex(I))ServerSwapSlots_Implementation(I,MainHandSlot,Id,Slots[MainHandSlot].InstanceId);}
void UBRInventoryComponent::SwapSlots(int32 From,int32 To)
{if(Slots.IsValidIndex(From) && Slots.IsValidIndex(To))ServerSwapSlots(From,To,Slots[From].InstanceId,Slots[To].InstanceId);}
void UBRInventoryComponent::StowHeld()
{for(int32 I=0;I<BackpackCapacity;++I)if(Slots[I].Item==EBRInventoryItem::Empty){SwapSlots(MainHandSlot,I);return;}}
void UBRInventoryComponent::ServerSwapSlots_Implementation(int32 From,int32 To,FGuid FromId,FGuid ToId)
{
    // Both identities are checked atomically; stale drags cannot replace newer items.
    if(!CanOperate() || From==To || !Slots.IsValidIndex(From) || !Slots.IsValidIndex(To) ||
       Slots[From].InstanceId!=FromId || Slots[To].InstanceId!=ToId || GetWorld()->GetTimeSeconds()<NextUseTime)return;
    if(Slots[From].Item==EBRInventoryItem::Empty && Slots[To].Item==EBRInventoryItem::Empty)return;
    const auto Old=EquippedItem;Swap(Slots[From],Slots[To]);Changed();
    if(Old!=EquippedItem)CastChecked<ABRPlayerCharacter>(GetOwner())->ClientPlayArmsAction(EquippedItem==EBRInventoryItem::Flashlight?1:EquippedItem==EBRInventoryItem::AlmondWater?4:6);
}
void UBRInventoryComponent::ServerUseSlot_Implementation(int32 I,FGuid Id)
{
    if(!CanOperate() || !Matches(I,Id) || GetWorld()->GetTimeSeconds()<NextUseTime)return;
    auto* P=CastChecked<ABRPlayerCharacter>(GetOwner());
    if(Slots[I].Item==EBRInventoryItem::AlmondWater)
    {
        if(Sanity>=99.9f)return;
        SelectedSlot=I;EquippedItem=EBRInventoryItem::AlmondWater;
        Sanity=FMath::Clamp(Sanity+AlmondWaterRestore,0.f,100.f);RemoveAt(I);
        NextUseTime=GetWorld()->GetTimeSeconds()+4.5;
        P->ClientPlayArmsAction(5);P->RefreshInventoryEquipment();
        UE_LOG(LogTemp,Display,TEXT("BR_INVENTORY result=CONSUMED item=AlmondWater sanity=%.2f"),Sanity);
    }
    else if(Slots[I].Item==EBRInventoryItem::Battery) {P->ReplaceBattery();NextUseTime=GetWorld()->GetTimeSeconds()+0.3;}
    else if(Slots[I].Item==EBRInventoryItem::Flashlight){SelectFirst(EBRInventoryItem::Flashlight);P->ToggleFlashlight();NextUseTime=GetWorld()->GetTimeSeconds()+0.3;}
}
void UBRInventoryComponent::ServerDropSlot_Implementation(int32 I,FGuid Id)
{
    if(!CanOperate() || !Matches(I,Id) || GetWorld()->GetTimeSeconds()<NextUseTime)return;
    auto* P=CastChecked<ABRPlayerCharacter>(GetOwner());FVector Eye;FRotator R;P->GetActorEyesViewPoint(Eye,R);
    const FVector Forward=FRotator(0,R.Yaw,0).Vector();FVector End=P->GetActorLocation()+Forward*90-FVector(0,0,45);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(BRDrop),true,P);FHitResult Hit;
    if(GetWorld()->SweepSingleByChannel(Hit,Eye,End,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(10),Q))
    {if(Hit.bStartPenetrating || Hit.Distance<40)return;End=Hit.Location;}
    if(GetWorld()->LineTraceSingleByChannel(Hit,End,End-FVector(0,0,250),ECC_Visibility,Q))End=Hit.ImpactPoint+FVector(0,0,5);
    else return; // Keep the item if no supported drop position exists.
    FTransform T(FRotator(0,R.Yaw,0),End);
    auto* Drop=GetWorld()->SpawnActorDeferred<ABRSupplyPickup>(ABRSupplyPickup::StaticClass(),T,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!Drop)return;
    Drop->SupplyType=Slots[I].Item==EBRInventoryItem::Flashlight?EBRSupplyType::Flashlight:Slots[I].Item==EBRInventoryItem::Battery?EBRSupplyType::Battery:EBRSupplyType::AlmondWater;
    Drop->StoredCharge=P->GetBatteryCharge();Drop->FinishSpawning(T);RemoveAt(I);P->RefreshInventoryEquipment();P->ClientPlayArmsAction(6);NextUseTime=GetWorld()->GetTimeSeconds()+0.3;
}
void UBRInventoryComponent::TickComponent(float DT,ELevelTick TT,FActorComponentTickFunction* F)
{
    Super::TickComponent(DT,TT,F);if(!GetOwner()->HasAuthority())return;SanityTick+=DT;if(SanityTick<1)return;
    auto* P=Cast<ABRPlayerCharacter>(GetOwner());auto* GS=GetWorld()->GetGameState<ABRGameState>();
    if(P && GS && GS->GetLevelPhase()==EBRLevelPhase::Exploring && !P->GetDownedComponent()->IsDowned() && (!bPauseDrainWhileHiding || !P->GetCurrentHideSpot()))
    {const float New=FMath::Clamp(Sanity-FMath::Max(0.f,SanityDrainPerSecond)*SanityTick,0.f,100.f);if(New!=Sanity){Sanity=New;Changed();}}
    SanityTick=0;
}
void UBRInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME_CONDITION(UBRInventoryComponent,Slots,COND_OwnerOnly);DOREPLIFETIME_CONDITION(UBRInventoryComponent,Sanity,COND_OwnerOnly);DOREPLIFETIME_CONDITION(UBRInventoryComponent,SelectedSlot,COND_OwnerOnly);DOREPLIFETIME(UBRInventoryComponent,EquippedItem);}
FText UBRInventoryComponent::ItemName(EBRInventoryItem I)
{return FText::FromString(I==EBRInventoryItem::Flashlight?TEXT("手电筒"):I==EBRInventoryItem::Battery?TEXT("电池组"):I==EBRInventoryItem::AlmondWater?TEXT("杏仁水"):TEXT("空格"));}
