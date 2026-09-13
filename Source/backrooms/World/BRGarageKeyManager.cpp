#include "World/BRGarageKeyManager.h"

#include "Core/BRGameState.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Net/UnrealNetwork.h"
#include "World/BRGarageDoor.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRGarageKeySocket.h"
#include "World/BRGarageExitDoor.h"
#include "World/BRLootCabinet.h"

namespace
{
	template <typename T>
	void ShuffleActors(TArray<T*>& Values, FRandomStream& Stream)
	{
		for (int32 Index = Values.Num() - 1; Index > 0; --Index)
		{
			Values.Swap(Index, Stream.RandRange(0, Index));
		}
	}
}

ABRGarageKeyManager::ABRGarageKeyManager()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
    bAlwaysRelevant = true;
	SetNetUpdateFrequency(5.0f);
}

void ABRGarageKeyManager::BeginPlay()
{
	Super::BeginPlay();
	if (!HasAuthority())
	{
		return;
	}

	const int32 Seed = RandomSeed == 0 ? FMath::Rand() : RandomSeed;
	FRandomStream Stream(Seed);
	ConfigureGarageDoors(Stream);
    if(!ConfigureCabinetLoot(Stream)) ConfigureKeyCandidates(Stream);
    // Add the test pickups after random loot selection, without registering extra objectives.
    if(bSpawnPointTestKeys && UsesKeySockets() && KeysRequired==4) SpawnPointTestKeys();
	UE_LOG(LogTemp, Display, TEXT("BR_GARAGE_RUNTIME result=READY seed=%d active_keys=%d required_keys=%d"), Seed, ActiveKeyCount, KeysRequired);
}

void ABRGarageKeyManager::SpawnPointTestKeys()
{
    APlayerStart* Start=nullptr;
    for(TActorIterator<APlayerStart> It(GetWorld());It;++It)
        if(!Start || It->GetName()<Start->GetName())Start=*It;
    if(!Start)
    {
        UE_LOG(LogTemp,Warning,TEXT("BR_SPAWN_TEST_KEYS result=SKIPPED reason=no_player_start"));
        return;
    }

    const FRotator Facing(0,Start->GetActorRotation().Yaw,0);
    const FVector Forward=Facing.Vector();
    const FVector Right=FRotationMatrix(Facing).GetUnitAxis(EAxis::Y);
    int32 Spawned=0;
    for(int32 Index=0;Index<4;++Index)
    {
        const FVector Above=Start->GetActorLocation()+Forward*240.f+Right*((Index-1.5f)*50.f);
        FHitResult Ground;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(BRSpawnTestKeys),true,this);
        if(!GetWorld()->LineTraceSingleByObjectType(Ground,Above,Above-FVector(0,0,350),
            FCollisionObjectQueryParams(ECC_WorldStatic),Query) || Ground.ImpactNormal.Z<0.7f)
        {
            UE_LOG(LogTemp,Warning,TEXT("BR_SPAWN_TEST_KEYS result=SKIPPED key=%d reason=no_floor"),Index+1);
            continue;
        }
        FActorSpawnParameters Params;
        Params.Owner=this;
        Params.Name=FName(*FString::Printf(TEXT("BR_SpawnTestKey_%d"),Index+1));
        Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        auto* Key=GetWorld()->SpawnActor<ABRGarageKeyPickup>(Ground.ImpactPoint,Facing,Params);
        if(!Key)continue;
        Key->SetActorScale3D(FVector(2));
        // Rest the visible mesh on the floor even if its imported pivot is not at the bottom.
        if(const auto* Mesh=Key->FindComponentByClass<UStaticMeshComponent>())
            Key->AddActorWorldOffset(FVector(0,0,Ground.ImpactPoint.Z+1.f-Mesh->Bounds.GetBox().Min.Z));
        Key->SetPickupActive(true);
        ++Spawned;
    }
    UE_LOG(LogTemp,Display,TEXT("BR_SPAWN_TEST_KEYS result=READY spawned=%d start=%s required=%d"),Spawned,*Start->GetName(),ActiveKeyCount);
}

void ABRGarageKeyManager::ConfigureKeyCandidates(FRandomStream& RandomStream)
{
	TArray<ABRGarageKeyPickup*> Candidates;
	for (TActorIterator<ABRGarageKeyPickup> It(GetWorld()); It; ++It)
	{
		Candidates.Add(*It);
	}
	ShuffleActors(Candidates, RandomStream);
	ActiveKeyCount = FMath::Clamp(KeysRequired, 0, Candidates.Num());
	CollectedKeys = 0;
	CountedPickups.Reset();
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		Candidates[Index]->SetPickupActive(Index < ActiveKeyCount);
	}

	if (ABRGameState* State = GetWorld()->GetGameState<ABRGameState>())
	{
		for (int32 Index = 0; Index < ActiveKeyCount; ++Index)
		{
			State->RegisterObjective();
		}
	}
	OnRep_KeyProgress();
}

void ABRGarageKeyManager::ConfigureGarageDoors(FRandomStream& RandomStream)
{
	TArray<ABRGarageDoor*> Doors;
	for (TActorIterator<ABRGarageDoor> It(GetWorld()); It; ++It)
	{
		if(!It->IsA<ABRLootCabinet>() && !It->IsA<ABRGarageExitDoor>()) Doors.Add(*It);
	}
	ShuffleActors(Doors, RandomStream);
	const int32 Available = FMath::Clamp(UnlockableDoorCount, 0, Doors.Num());
	for (int32 Index = 0; Index < Doors.Num(); ++Index)
	{
		Doors[Index]->SetUnlockable(Index < Available);
	}
	UE_LOG(LogTemp, Display, TEXT("BR_GARAGE_DOORS result=READY total=%d unlockable=%d"), Doors.Num(), Available);
}

bool ABRGarageKeyManager::ConfigureCabinetLoot(FRandomStream& Stream)
{
    TArray<ABRLootCabinet*> Cabinets;
    TArray<ABRLootCabinet*> Eligible;
    for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)
    {Cabinets.Add(*It);if(It->bAllowObjectiveKey)Eligible.Add(*It);}
    if(Eligible.Num()<KeysRequired)return false; // Preserve existing loose-key maps without enough configured cabinets.
    for(TActorIterator<ABRGarageKeyPickup> It(GetWorld());It;++It)It->SetPickupActive(false);
    Eligible.Sort([](const ABRLootCabinet& A,const ABRLootCabinet& B){return A.GetPathName()<B.GetPathName();});
    ShuffleActors(Eligible,Stream);
    ActiveKeyCount=0;CollectedKeys=0;CountedPickups.Reset();
    for(int32 I=0;I<KeysRequired;++I)
    {
        auto* Cabinet=Eligible[I];Cabinet->ConfigureLoot(1);
        if(Cabinet->GetKey())++ActiveKeyCount;
        // Never place a required key behind a randomly locked access door.
        if(!Cabinet->RequiredDoorTag.IsNone())
            for(TActorIterator<ABRGarageDoor> Door(GetWorld());Door;++Door)
                if(!Door->IsA<ABRLootCabinet>() && !Door->IsA<ABRGarageExitDoor>() && Door->ControlledActorTag==Cabinet->RequiredDoorTag)Door->SetUnlockable(true);
    }
    for(auto* Cabinet:Cabinets)if(Cabinet->GetLootType()<0)
    {
        const float Draw=Stream.FRand();
        const float Battery=FMath::Clamp(Cabinet->BatteryChance,0.f,1.f);
        Cabinet->ConfigureLoot(Draw<Battery?2:Draw<FMath::Min(1.f,Battery+FMath::Max(0.f,Cabinet->FlashlightChance))?3:Draw<FMath::Min(1.f,Battery+FMath::Max(0.f,Cabinet->FlashlightChance)+FMath::Max(0.f,Cabinet->AlmondWaterChance))?4:0);
    }
    if(auto* State=GetWorld()->GetGameState<ABRGameState>())for(int32 I=0;I<ActiveKeyCount;++I)State->RegisterObjective();
    OnRep_KeyProgress();ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("BR_LOOT result=CONFIGURED cabinets=%d keys=%d required=%d"),Cabinets.Num(),ActiveKeyCount,KeysRequired);
    return true;
}

void ABRGarageKeyManager::HandleKeyCollected(ABRGarageKeyPickup* Pickup, APawn*)
{
	if (!HasAuthority() || !IsValid(Pickup) || !Pickup->IsCollected() || CountedPickups.Contains(Pickup))
	{
		return;
	}
	CountedPickups.Add(Pickup);
	CollectedKeys = FMath::Clamp(CollectedKeys + 1, 0, ActiveKeyCount);
    if(!UsesKeySockets())
        if(auto* State=GetWorld()->GetGameState<ABRGameState>())State->NotifyObjectiveCompleted();
	OnRep_KeyProgress();
	ForceNetUpdate();
}

void ABRGarageKeyManager::OnRep_KeyProgress()
{
	OnGarageKeysChanged.Broadcast(CollectedKeys, ActiveKeyCount);
}

void ABRGarageKeyManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABRGarageKeyManager, ActiveKeyCount);
	DOREPLIFETIME(ABRGarageKeyManager, CollectedKeys);
    DOREPLIFETIME(ABRGarageKeyManager, InsertedKeys);
}

bool ABRGarageKeyManager::AreAllKeysInserted() const
{return UsesKeySockets() && KeysRequired==4 && KeySockets.Num()==4 && InsertedKeys==4;}
bool ABRGarageKeyManager::CanInsertKey(const ABRGarageKeySocket* Socket) const
{
    const auto* State=GetWorld()->GetGameState<ABRGameState>();
    return IsValid(Socket) && Socket->KeyManager==this && KeySockets.Contains(Socket) && KeySockets.Num()==4 &&
        KeysRequired==4 && ActiveKeyCount==4 && CollectedKeys==4 && InsertedKeys<4 && !Socket->IsInserted() &&
        IsValid(ExitDoor) && State && State->GetLevelPhase()==EBRLevelPhase::Exploring;
}
bool ABRGarageKeyManager::TryInsertKey(ABRGarageKeySocket* Socket)
{
    if(!HasAuthority() || !CanInsertKey(Socket))return false;
    // A single server transaction spends one shared key on one still-empty socket.
    Socket->MarkInserted();++InsertedKeys;
    if(auto* State=GetWorld()->GetGameState<ABRGameState>())State->NotifyObjectiveCompleted();
    OnRep_KeyProgress();ForceNetUpdate();
    if(AreAllKeysInserted())ExitDoor->OpenAfterKeysInserted();
    UE_LOG(LogTemp,Display,TEXT("BR_KEY_INSERT socket=%d inserted=%d required=4"),Socket->SocketNumber,InsertedKeys);
    return true;
}
