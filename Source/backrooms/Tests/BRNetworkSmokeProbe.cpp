#include "Tests/BRNetworkSmokeProbe.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRPlayerState.h"
#include "World/BRHideSpot.h"
#include "World/BRGarageDoor.h"
#include "World/BRLootCabinet.h"
#include "World/BRSupplyPickup.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRGarageKeyManager.h"
#include "AI/BREntityCharacter.h"
#include "EngineUtils.h"
#include "World/BRExtractionZone.h"
#include "Player/BRDownedComponent.h"
#include "Core/BRGameState.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

ABRNetworkSmokeProbe::ABRNetworkSmokeProbe() { PrimaryActorTick.bCanEverTick = true; PrimaryActorTick.TickInterval = 0.2f; }
void ABRNetworkSmokeProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if (Started==0) Started=FPlatformTime::Seconds();
    const double T=FPlatformTime::Seconds()-Started;
    auto Check = [](bool bOK,const TCHAR* Case) { UE_LOG(LogTemp,Display,TEXT("BR_GAMEPLAY_SMOKE case=%s result=%s"),Case,bOK?TEXT("PASS"):TEXT("FAIL")); };
    if (Stage==0 && T>2)
    {
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld());It;++It)
            if (auto* PS=It->GetPlayerState<ABRPlayerState>()) if (PS->IsRoomOwner()) TestPlayer=*It;
        for (TActorIterator<ABRHideSpot> It(GetWorld());It;++It) { if(!It->Cabinet){Spot=*It;break;} }
        for (TActorIterator<ABREntityCharacter> It(GetWorld());It;++It) { Entity=*It;EntityStart=It->GetActorLocation();break; }
        // Regression fixture only: the shipping map uses room walls/doors for concealment,
        // not a cabinet teleport spot. Keep the generic replicated hide-state contract tested.
        if(TestPlayer.IsValid() && !Spot.IsValid())
        {Spot=GetWorld()->SpawnActor<ABRHideSpot>(TestPlayer->GetActorLocation()-FVector(0,0,96),FRotator::ZeroRotator);UE_LOG(LogTemp,Display,TEXT("BR_GAMEPLAY_SMOKE hiding_fixture=non_cabinet"));}
        if (!TestPlayer.IsValid() || !Spot.IsValid()) return;
        if(Spot->Cabinet && Spot->Cabinet->GetOpenAlpha()<0.99f){Spot->Cabinet->SetDoorOpen(true);return;}
        Spot->Interact_Implementation(TestPlayer.Get()); Check(TestPlayer->GetCurrentHideSpot()==Spot.Get(),TEXT("HIDE_ENTER")); Stage=1;
    }
    if (Stage==1 && T>6)
    {
        Spot->Interact_Implementation(TestPlayer.Get()); Check(!TestPlayer->GetCurrentHideSpot() && !TestPlayer->IsHidden(),TEXT("HIDE_EXIT"));
        for(TActorIterator<ABRLootCabinet> C(GetWorld());C;++C)C->SetDoorOpen(true);
        Stage=2;
    }
    if (Stage==2 && T>8)
    {
        ABRGarageKeyManager* M=nullptr; for (TActorIterator<ABRGarageKeyManager> It(GetWorld());It;++It) { M=*It;break; }
        if (M) for (TActorIterator<ABRGarageKeyPickup> It(GetWorld());It;++It) if (It->IsPickupActive())
        {
            int32 Before=M->GetCollectedKeys(); It->Interact_Implementation(TestPlayer.Get()); It->Interact_Implementation(TestPlayer.Get());
            Check(M->GetCollectedKeys()==Before+1,TEXT("KEY_ONCE"));break;
        }
        for (TActorIterator<ABRGarageDoor> It(GetWorld());It;++It) if (!It->IsA<ABRLootCabinet>() && It->CanInteract_Implementation(TestPlayer.Get()))
        { TestedDoor=*It;It->Interact_Implementation(TestPlayer.Get());Check(It->IsOpen(),TEXT("DOOR_OPEN"));break; }
        for(TActorIterator<ABRSupplyPickup> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("BR_EntryFlashlight")))
        {
            It->Interact_Implementation(TestPlayer.Get());Check(It->IsCollected() && TestPlayer->HasFlashlight(),TEXT("FLASHLIGHT_COLLECT"));
            for(TActorIterator<ABRPlayerCharacter> Other(GetWorld());Other;++Other)if(*Other!=TestPlayer.Get())
            {It->Interact_Implementation(*Other);Check(!Other->HasFlashlight(),TEXT("FLASHLIGHT_SINGLE_OWNER"));}break;
        }
        if(FParse::Param(FCommandLine::Get(),TEXT("BRInventoryNet")))for(TActorIterator<ABRPlayerCharacter> Other(GetWorld());Other;++Other)if(*Other!=TestPlayer.Get())
        {Other->GrantBattery();for(int32 I=0;I<11;++I)Other->GrantAlmondWater();Check(Other->GetInventoryComponent()->GetUsedSlots()==12,TEXT("INVENTORY_SERVER_TWELVE"));}
        Stage=3;
    }
    if (Stage==3 && T>13)
    {
        Check(Entity.IsValid() && FVector::Dist2D(EntityStart,Entity->GetActorLocation())>30,TEXT("AI_MOVED"));
        if(FParse::Param(FCommandLine::Get(),TEXT("BRInventoryNet")))for(TActorIterator<ABRPlayerCharacter> Other(GetWorld());Other;++Other)if(*Other!=TestPlayer.Get())
        {Check(Other->GetInventoryComponent()->GetUsedSlots()==11 && Other->GetInventoryComponent()->Count(EBRInventoryItem::AlmondWater)==10,TEXT("INVENTORY_CLIENT_RPC_CONSUMED_ONCE"));Check(Other->GetInventoryComponent()->GetSanity()>99 && Other->GetInventoryComponent()->GetSanity()<=100,TEXT("INVENTORY_SERVER_SAN_RECOVERED"));}
        if(TestedDoor.IsValid()){TestedDoor->SetDoorOpen(false);Check(!TestedDoor->IsOpen(),TEXT("DOOR_CLOSE"));}
        TestPlayer->ToggleFlashlight();Check(!TestPlayer->IsFlashlightOn(),TEXT("FLASHLIGHT_OFF"));Stage=4;
    }
    if(Stage==4 && T>14 && !bBatteryTested)
    {
        TestPlayer->GrantBattery();TestPlayer->ReplaceBattery();Check(TestPlayer->GetBatteryCharge()==100 && TestPlayer->GetSpareBatteries()==0,TEXT("BATTERY_RELOAD"));bBatteryTested=true;
    }
    if (Stage==4 && T>16 && FParse::Param(FCommandLine::Get(),TEXT("BRRealRoundSmoke")))
    {
        TestPlayer->ToggleFlashlight();Check(TestPlayer->IsFlashlightOn(),TEXT("FLASHLIGHT_ON"));
        if (FParse::Param(FCommandLine::Get(),TEXT("BRSmokeAllDown")))
        {
            for (TActorIterator<ABRPlayerCharacter> It(GetWorld());It;++It) It->GetDownedComponent()->Down();
        }
        else
        {
            for (TActorIterator<ABRGarageKeyPickup> It(GetWorld());It;++It)
                if (It->IsPickupActive()) It->Interact_Implementation(TestPlayer.Get());
            for (TActorIterator<ABRExtractionZone> Zone(GetWorld());Zone;++Zone)
            {
                int32 I=0;
                for (TActorIterator<ABRPlayerCharacter> It(GetWorld());It;++It,++I)
                    It->SetActorLocation(Zone->GetActorLocation()+FVector((I%2)*110-55,(I/2)*110-55,60),false,nullptr,ETeleportType::TeleportPhysics);
                break;
            }
        }
        Stage=5;
    }
    if (Stage==5 && T>18)
    {
        const auto* GS=GetWorld()->GetGameState<ABRGameState>();
        const bool bDown=FParse::Param(FCommandLine::Get(),TEXT("BRSmokeAllDown"));
        Check(GS && GS->GetLevelPhase()==(bDown?EBRLevelPhase::Failed:EBRLevelPhase::Completed),bDown?TEXT("ALL_DOWN_END"):TEXT("EXTRACTION_OVERLAP_END"));
        Stage=6;
    }
#endif
}
