#include "Tests/BRItemSmokeProbe.h"
#include "Player/BRPlayerCharacter.h"
#include "UI/BRMenuPlayerController.h"
#include "UI/BRGameHUDWidget.h"
#include "Player/BRInventoryComponent.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "World/BRGarageDoor.h"
#include "World/BRLootCabinet.h"
#include "World/BRSupplyPickup.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRGarageKeyManager.h"
#include "Interaction/BRInteractionComponent.h"
#include "AI/BREntityCharacter.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
ABRItemSmokeProbe::ABRItemSmokeProbe(){PrimaryActorTick.bCanEverTick=true;}
void ABRItemSmokeProbe::Check(bool OK,const TCHAR* Name)
{++Checks;if(!OK)++Failures;UE_LOG(LogTemp,Display,TEXT("BR_ITEMS_SMOKE case=%s result=%s"),Name,OK?TEXT("PASS"):TEXT("FAIL"));}
void ABRItemSmokeProbe::Aim(FVector Point)
{
    // Align the body first: the camera's -10 cm offset rotates with the capsule.
    // Otherwise teleporting between test cabinets aims using the previous body's camera offset.
    Player->SetActorRotation(FRotator(0,(Point-Player->GetActorLocation()).Rotation().Yaw,0));
    FVector Eye;FRotator Rotation;Player->GetActorEyesViewPoint(Eye,Rotation);
    Player->GetController()->SetControlRotation((Point-Eye).Rotation());
}
void ABRItemSmokeProbe::Stand(AActor* Target,FVector Offset)
{Player->SetActorLocation(Target->GetActorTransform().TransformPositionNoScale(Offset),false,nullptr,ETeleportType::TeleportPhysics);}
void ABRItemSmokeProbe::DebugAim(AActor* Target)
{
    FVector Eye;FRotator R;Player->GetActorEyesViewPoint(Eye,R);
    UE_LOG(LogTemp,Display,TEXT("BR_ITEM_TRACE target=%s position=%s eye=%s"),*GetPathNameSafe(Target),*Target->GetActorLocation().ToCompactString(),*Eye.ToCompactString());
    for(bool Complex:{false,true})
    {
        FCollisionQueryParams P(SCENE_QUERY_STAT(BRItemProbe),Complex,Player);
        for(int32 I=0;I<5;++I)
        {
            FHitResult H;if(!GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+R.Vector()*350,ECC_Visibility,P))break;
            UE_LOG(LogTemp,Display,TEXT("BR_ITEM_TRACE complex=%d index=%d hit=%s component=%s pos=%s"),Complex,I,*GetPathNameSafe(H.GetActor()),*GetNameSafe(H.GetComponent()),*H.ImpactPoint.ToCompactString());
            P.AddIgnoredActor(H.GetActor());
        }
    }
}
void ABRItemSmokeProbe::Capture(const TCHAR* Name)
{
    if(Captured.Contains(Name))return;Captured.Add(Name);
    FString Path;FParse::Value(FCommandLine::Get(),TEXT("BRItemCapture="),Path);
    if(!Path.IsEmpty())FScreenshotRequest::RequestScreenshot(FPaths::Combine(Path,FString(Name)+TEXT(".png")),true,false);
}
void ABRItemSmokeProbe::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
#if !UE_BUILD_SHIPPING
    if(Started==0)Started=FPlatformTime::Seconds();
    const double Time=FPlatformTime::Seconds()-Started;
    if(Stage==1 && Time>3)Capture(TEXT("01_hands"));
    if(Stage==3 && Time>7)Capture(TEXT("02_flashlight"));
    if(Stage==5 && Time>11)Capture(TEXT("03_door_open"));
    if(Stage==7 && Time>15)Capture(TEXT("04_door_closed"));
    if(Stage==10 && Time>21)Capture(TEXT("05_cabinet_loot"));
    if(Stage==14)
    {
        if(Time<NextAuditTime)return;
        if(AuditIndex>=AuditCabinets.Num())
        {
            UE_LOG(LogTemp,Display,TEXT("BR_ITEMS_SMOKE result=%s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);
            FPlatformMisc::RequestExit(false);Stage=15;return;
        }
        auto* C=AuditCabinets[AuditIndex].Get();
        if(AuditPhase==0)
        {
            Stand(C,FVector(150,0,90));C->SetDoorOpen(false);
            Aim(C->GetActorTransform().TransformPosition(FVector(28,0,C->bSliding?5:120)));
        }
        if(AuditPhase==1)
        {
            auto* Focus=Player->GetInteractionComponent()->FindFocusedInteractable();
            Check(Focus==C,*FString::Printf(TEXT("CABINET_%02d_AIM"),AuditIndex));
            if(Focus!=C){UE_LOG(LogTemp,Display,TEXT("BR_ITEMS_SMOKE target=%s actual=%s"),*C->GetPathName(),*GetPathNameSafe(Focus));DebugAim(C);}
            CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();
        }
        if(AuditPhase==2)
        {
            Check(C->IsOpen() && C->GetOpenAlpha()>0.99f,*FString::Printf(TEXT("CABINET_%02d_OPEN"),AuditIndex));
            if(auto* K=C->GetKey())if(!K->IsCollected())Aim(K->GetActorLocation());
        }
        if(AuditPhase==3)
        {
            if(auto* K=C->GetKey())if(!K->IsCollected())
            {CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();Check(K->IsCollected(),*FString::Printf(TEXT("CABINET_%02d_KEY"),AuditIndex));if(!K->IsCollected())DebugAim(K);}
            Stand(C,FVector(250,0,90));
            C->SetDoorOpen(false);
            Check(!C->IsOpen(),*FString::Printf(TEXT("CABINET_%02d_CLOSE"),AuditIndex));
            ++AuditIndex;
        }
        AuditPhase=(AuditPhase+1)%4;NextAuditTime=Time+1.1;return;
    }
    if(Stage>14 || Time<2+Stage*2)return;
    if(Stage==0)
    {
        Player=Cast<ABRPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!Player)return;
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        for(TActorIterator<ABREntityCharacter> It(GetWorld());It;++It)if(auto* AI=Cast<AAIController>(It->GetController())){AI->StopMovement();AI->UnPossess();}
        for(TActorIterator<ABRSupplyPickup> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("BR_EntryFlashlight"))){Torch=*It;break;}
        for(TActorIterator<ABRGarageDoor> It(GetWorld());It;++It)if(!It->IsA<ABRLootCabinet>()){Door=*It;break;}
        for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)if(It->GetKey()){Cabinet=*It;break;}
        Check(Torch && Door && Cabinet,TEXT("MAP_BINDINGS"));
        if(!Torch || !Door || !Cabinet){Stage=99;FPlatformMisc::RequestExit(false);return;}
        Player->SetActorLocation(FVector(3970,2100,3225),false,nullptr,ETeleportType::TeleportPhysics);Aim(FVector(2800,2100,3280));
    }
    if(Stage==1){Capture(TEXT("01_hands"));Stand(Torch,FVector(115,0,90));Aim(Torch->GetActorLocation());}
    if(Stage==2){CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();Check(Player->HasFlashlight() && Torch->IsCollected(),TEXT("LMB_FLASHLIGHT_PICKUP"));Player->SetActorLocation(FVector(3970,2100,3225));Aim(FVector(2800,2100,3280));}
    if(Stage==3)
    {
        Capture(TEXT("02_flashlight"));Door->SetUnlockable(true);Stand(Door,FVector(160,50,90));Aim(Door->GetActorTransform().TransformPosition(FVector(0,50,120)));
        for(TActorIterator<AActor> It(GetWorld());It;++It)if(Door->OwnsPanel(*It)){Leaf=*It;LeafClosed=Leaf->GetActorTransform();break;}
    }
    if(Stage==4){
        TArray<UUserWidget*> HUDs;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,HUDs,UBRGameHUDWidget::StaticClass(),true);
        Check(HUDs.Num()==1 && Cast<UBRGameHUDWidget>(HUDs[0])->HasInteractableFocus(),TEXT("FOCUSED_DOOR_RETICLE_CIRCLE"));
        auto* Inv=Player->GetInventoryComponent();Player->GrantAlmondWater();for(int32 I=0;I<12;++I)if(Inv->GetSlots()[I].Item==EBRInventoryItem::AlmondWater){Inv->SelectSlot(I);break;}
        const int32 WaterCount=Inv->Count(EBRInventoryItem::AlmondWater);
CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();Check(Door->IsOpen(),TEXT("LMB_DOOR_OPEN"));Check(Inv->Count(EBRInventoryItem::AlmondWater)==WaterCount,TEXT("DOOR_CLICK_DOES_NOT_DRINK"));Check(Door->GetOpenAlpha()<0.9f,TEXT("DOOR_NOT_INSTANT"));}
    if(Stage==5)
    {
        Check(Door->GetOpenAlpha()>0.99f,TEXT("DOOR_REACHES_OPEN"));Capture(TEXT("03_door_open"));
        if(Leaf){auto* M=Leaf->FindComponentByClass<UStaticMeshComponent>();Aim(Leaf->GetActorTransform().TransformPosition(M->GetStaticMesh()->GetBoundingBox().GetCenter()));}
    }
    if(Stage==6){CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();Check(!Door->IsOpen(),TEXT("LMB_DOOR_CLOSE"));}
    if(Stage==7)
    {
        Check(Door->GetOpenAlpha()<0.01f && Leaf && Leaf->GetActorTransform().Equals(LeafClosed,0.05f),TEXT("DOOR_RESTORED_TRANSFORM"));Capture(TEXT("04_door_closed"));
        Stand(Cabinet,FVector(140,20,90));Aim(Cabinet->GetActorTransform().TransformPosition(FVector(28,0,Cabinet->bSliding?5:120)));
        for(TActorIterator<ABRGarageKeyManager> It(GetWorld());It;++It){KeysBefore=It->GetCollectedKeys();Check(It->GetRequiredKeys()==4,TEXT("FOUR_GUARANTEED_KEYS"));}
    }
    if(Stage==8){UE_LOG(LogTemp,Display,TEXT("BR_ITEMS_SMOKE cabinet=%s focus=%s"),*Cabinet->GetPathName(),*GetNameSafe(Player->GetInteractionComponent()->FindFocusedInteractable()));CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();Check(Cabinet->IsOpen(),TEXT("LMB_CABINET_OPEN"));}
    if(Stage==9)
    {
        Player->GetInventoryComponent()->SelectFirst(EBRInventoryItem::Flashlight);Player->ToggleFlashlight();
        auto* K=Cabinet->GetKey();Check(K && K->IsPickupActive(),TEXT("LOOT_REVEALED_AFTER_OPEN"));
        Cabinet->ConfigureLoot(2);Check(Cabinet->GetKey()==K && Cabinet->GetLootType()==1,TEXT("NO_REROLL"));
        if(K)Aim(K->GetActorLocation());
    }
    if(Stage==10)
    {
        UE_LOG(LogTemp,Display,TEXT("BR_ITEMS_SMOKE key_focus=%s"),*GetNameSafe(Player->GetInteractionComponent()->FindFocusedInteractable()));
        CastChecked<ABRMenuPlayerController>(Player->GetController())->UseHeldItem();auto* K=Cabinet->GetKey();Check(K && K->IsCollected(),TEXT("LMB_CABINET_KEY_PICKUP"));
        if(K && !K->IsCollected())DebugAim(K);
        if(K)K->Interact_Implementation(Player);
        for(TActorIterator<ABRGarageKeyManager> It(GetWorld());It;++It)Check(It->GetCollectedKeys()==KeysBefore+1,TEXT("KEY_COUNTS_ONCE"));
        Player->GrantBattery();Player->ReplaceBattery();Check(Player->GetSpareBatteries()==0 && Player->GetBatteryCharge()==100,TEXT("BATTERY_RELOAD"));
    }
    if(Stage==11){Player->ToggleFlashlight();Check(!Player->IsFlashlightOn(),TEXT("FLASHLIGHT_OFF"));}
    if(Stage==12){Player->ToggleFlashlight();Check(Player->IsFlashlightOn(),TEXT("FLASHLIGHT_ON"));}
    if(Stage==13)
    {
        for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)AuditCabinets.Add(*It);
        NextAuditTime=Time+1;
    }
    ++Stage;
#endif
}
