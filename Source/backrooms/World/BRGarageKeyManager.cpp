#include "World/BRGarageKeyManager.h"

#include "Core/BRGameState.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "World/BRGarageDoor.h"
#include "World/BRGarageKeyPickup.h"
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
	UE_LOG(LogTemp, Display, TEXT("BR_GARAGE_RUNTIME result=READY seed=%d active_keys=%d required_keys=%d"), Seed, ActiveKeyCount, KeysRequired);
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
		if(!It->IsA<ABRLootCabinet>()) Doors.Add(*It);
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
                if(!Door->IsA<ABRLootCabinet>() && Door->ControlledActorTag==Cabinet->RequiredDoorTag)Door->SetUnlockable(true);
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
	if (!HasAuthority() || !IsValid(Pickup) || CountedPickups.Contains(Pickup))
	{
		return;
	}
	CountedPickups.Add(Pickup);
	CollectedKeys = FMath::Clamp(CollectedKeys + 1, 0, ActiveKeyCount);
	if (ABRGameState* State = GetWorld()->GetGameState<ABRGameState>())
	{
		State->NotifyObjectiveCompleted();
	}
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
}
