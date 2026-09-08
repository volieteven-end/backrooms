#include "Tests/BRInventorySmokeProbe.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRFirstPersonArmsComponent.h"
#include "Player/BRDownedComponent.h"
#include "Player/BRStaminaComponent.h"
#include "UI/BRMenuPlayerController.h"
#include "UI/BRInventoryWidget.h"
#include "World/BRHideSpot.h"
#include "World/BRLootCabinet.h"
#include "World/BRSupplyPickup.h"
#include "AI/BREntityCharacter.h"
#include "AIController.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Camera/CameraTypes.h"
#include "TimerManager.h"
#include "Input/DragAndDrop.h"
ABRInventorySmokeProbe::ABRInventorySmokeProbe(){PrimaryActorTick.bCanEverTick=true;}
void ABRInventorySmokeProbe::Check(bool OK,const TCHAR* N){++Checks;if(!OK)++Failures;UE_LOG(LogTemp,Display,TEXT("BR_INVENTORY_SMOKE case=%s result=%s"),N,OK?TEXT("PASS"):TEXT("FAIL"));}
void ABRInventorySmokeProbe::Capture(const TCHAR* N)
{FString Dir;FParse::Value(FCommandLine::Get(),TEXT("BRInventoryCapture="),Dir);if(!Dir.IsEmpty())FScreenshotRequest::RequestScreenshot(FPaths::Combine(Dir,FString(N)+TEXT(".png")),true,false);}
void ABRInventorySmokeProbe::Stand(FVector L,float Yaw)
{P->GetCharacterMovement()->StopMovementImmediately();P->TeleportTo(L,FRotator(0,Yaw,0),false,true);PC->SetControlRotation(FRotator(0,Yaw,0));P->GetCharacterMovement()->SetMovementMode(MOVE_Walking);}
void ABRInventorySmokeProbe::StepFixture(float Height)
{
    if(!Curb)
    {
        auto* Floor=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-20000,-20000,3980),FRotator::ZeroRotator);
        Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Floor->SetActorScale3D(FVector(50,6,0.4));
        Curb=GetWorld()->SpawnActor<AStaticMeshActor>();Curb->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Curb->GetStaticMeshComponent()->SetStaticMesh(Floor->GetStaticMeshComponent()->GetStaticMesh());Curb->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
    }
    Curb->SetActorLocation(FVector(-19850,-20000,4000+Height/2));Curb->SetActorScale3D(FVector(3,5,Height/100));
    Stand(FVector(-20160,-20000,4000+P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+3));
}
void ABRInventorySmokeProbe::Tick(float DT)
{
    Super::Tick(DT);
#if !UE_BUILD_SHIPPING
    if(FParse::Param(FCommandLine::Get(),TEXT("BRArmsViews"))){TickArmsViews();return;}
    if(Started==0)Started=FPlatformTime::Seconds();const double T=FPlatformTime::Seconds()-Started;
    if(P)
    {
        if(Stage==1)P->AddMovementInput(FVector(-1,0,0),1);
        if((Stage>=13 && Stage<=16) || Stage==18 || Stage==19)P->AddMovementInput(FVector(1,0,0),1);
        if(Stage==2 && T>5.2 && T<5.3)Capture(TEXT("02_torch_hold"));
    }
    if(Stage>21 || T<2+Stage*2)return;
    if(Stage==0)
    {
        P=Cast<ABRPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P)return;PC=Cast<ABRMenuPlayerController>(P->GetController());if(!PC)return;Bag=P->GetInventoryComponent();
        TArray<USkeletalMeshComponent*> Ms;P->GetComponents(Ms);for(auto* M:Ms)if(M->GetFName()==TEXT("FirstPersonArms"))Arms=M;
        for(TActorIterator<ABREntityCharacter> It(GetWorld());It;++It)if(auto* AI=Cast<AAIController>(It->GetController())){AI->StopMovement();AI->UnPossess();}
        Check(Bag && Bag->GetSlots().Num()==12 && Bag->GetUsedSlots()==0,TEXT("TWELVE_EMPTY_SLOTS"));Check(Bag->GetSanity()>99 && Bag->GetSanity()<=100,TEXT("SAN_INITIAL"));
        FMinimalViewInfo View;P->CalcCamera(0,View);Check(FMath::IsNearlyEqual(View.PerspectiveNearClipPlane,1.f),TEXT("FIRST_PERSON_NEAR_PLANE"));
        Check(Arms->IsVisible() && !Arms->IsBoneHiddenByName(TEXT("LeftShoulder")),TEXT("EMPTY_HANDS_VISIBLE"));
        int32 Disabled=0,Rooms=0;
        for(TActorIterator<ABRHideSpot> H(GetWorld());H;++H){if(H->Cabinet){++Disabled;Check(!H->CanInteract_Implementation(P),TEXT("CABINET_HIDE_DISABLED"));}else ++Rooms;}
        UE_LOG(LogTemp,Display,TEXT("BR_HIDE_AUDIT cabinet_disabled=%d room_spots=%d"),Disabled,Rooms);
        Stand(FVector(4380,2109,3215),180);Capture(TEXT("01_empty_arms"));
    }
    if(Stage==1)
    {
        Check(P->GetActorLocation().X<4200,TEXT("ACTUAL_ELEVATOR_EXIT_WALK"));UE_LOG(LogTemp,Display,TEXT("BR_MOVEMENT actual_exit=%s"),*P->GetActorLocation().ToCompactString());
        if(P->GetActorLocation().X>=4200){FHitResult H;FCollisionQueryParams Q(SCENE_QUERY_STAT(BRStepTest),false,P);GetWorld()->SweepSingleByChannel(H,P->GetActorLocation(),P->GetActorLocation()+FVector(-200,0,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96),Q);UE_LOG(LogTemp,Display,TEXT("BR_MOVEMENT blocked=%s normal=%s"),*GetPathNameSafe(H.GetActor()),*H.ImpactNormal.ToCompactString());}
        Stand(FVector(3990,2160,3214),180);Check(P->GrantFlashlight(),TEXT("TORCH_STORED"));Check(Bag->GetUsedSlots()==1 && Bag->Count(EBRInventoryItem::Flashlight)==1,TEXT("TORCH_SLOT"));
    }
    if(Stage==2)
    {
        Check(P->GetArmsAnimationComponent()->GetAnimationState()==TEXT("FlashlightHold"),TEXT("TORCH_HOLD_STATE"));
        HandBefore=Arms->GetSocketTransform(TEXT("RightHand"),RTS_Component).GetLocation();Capture(TEXT("02_torch_hold"));
        P->ToggleFlashlight();Check(!P->IsFlashlightOn(),TEXT("TORCH_TOGGLE"));
    }
    if(Stage==3)
    {
        P->ToggleFlashlight();
        Check(Arms->GetSingleNodeInstance() && Arms->GetSingleNodeInstance()->IsPlaying(),TEXT("ARM_CLIP_ADVANCES"));
        for(int32 I=0;I<5;++I)Check(P->GrantBattery(),*FString::Printf(TEXT("BATTERY_SLOT_%d"),I));
        for(int32 I=0;I<6;++I)Check(P->GrantAlmondWater(),TEXT("WATER_STORED"));Check(Bag->GetUsedSlots()==12,TEXT("FULL_TWELVE_SLOTS"));
        auto* Extra=GetWorld()->SpawnActor<ABRSupplyPickup>();Extra->SupplyType=EBRSupplyType::AlmondWater;Extra->Interact_Implementation(P);Check(!Extra->IsCollected() && !P->GrantAlmondWater(),TEXT("FULL_BAG_PICKUP_REJECTED"));Extra->Destroy();
        SanityBefore=Bag->GetSanity();Bag->SanityDrainPerSecond=1;PC->ToggleInventory();Check(PC->IsInventoryOpen() && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored(),TEXT("INVENTORY_INPUT_MODE"));
    }
    if(Stage==4)
    {
        Capture(TEXT("03_inventory_full"));TArray<UUserWidget*> Ws;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Ws,UBRInventoryWidget::StaticClass(),true);
        int32 Buttons=0;for(auto* W:Ws)for(int32 I=0;I<12;++I)if(Cast<UButton>(W->WidgetTree->FindWidget(FName(*FString::Printf(TEXT("Slot%d"),I)))))++Buttons;
        Check(Buttons==12,TEXT("EDITABLE_TWELVE_BUTTONS"));
        if(Ws.Num()==1)
        {
            auto* UI=CastChecked<UBRInventoryWidget>(Ws[0]);auto* From=UI->WidgetTree->FindWidget(TEXT("Slot0"));auto* To=UI->WidgetTree->FindWidget(TEXT("Slot1"));
            const auto Id=Bag->GetSlots()[0].InstanceId;TSet<FKey> ButtonsDown;ButtonsDown.Add(EKeys::LeftMouseButton);
            const auto XY=From->GetCachedGeometry().GetAbsolutePosition()+From->GetCachedGeometry().GetAbsoluteSize()*0.5f;
            const auto Dest=To->GetCachedGeometry().GetAbsolutePosition()+To->GetCachedGeometry().GetAbsoluteSize()*0.5f;
            FPointerEvent Down(0,XY,XY,ButtonsDown,EKeys::LeftMouseButton,0,FModifierKeysState());
            Check(UI->NativeOnMouseButtonDown(UI->GetCachedGeometry(),Down).IsEventHandled(),TEXT("UMG_MOUSE_DOWN"));
            UDragDropOperation* Op=nullptr;UI->NativeOnDragDetected(UI->GetCachedGeometry(),Down,Op);Check(Op!=nullptr,TEXT("UMG_DRAG_CREATED"));
            FPointerEvent Release(0,Dest,XY,TSet<FKey>(),EKeys::LeftMouseButton,0,FModifierKeysState());FDragDropEvent DropEvent(Release,nullptr);
            Check(Op && UI->NativeOnDrop(UI->GetCachedGeometry(),DropEvent,Op),TEXT("UMG_DROP_ACCEPTED"));
            Check(Bag->GetSlots()[1].InstanceId==Id,TEXT("UMG_DRAG_MOVES_REAL_ITEM"));Bag->SwapSlots(0,1);
        }
        // Real storage in both pockets and main hand, with stale destination protection.
        auto Old0=Bag->GetSlots()[0],Old1=Bag->GetSlots()[1];Bag->SwapSlots(0,1);
        Check(Bag->GetSlots()[1].InstanceId==Old0.InstanceId && Bag->GetSlots()[0].InstanceId==Old1.InstanceId,TEXT("ATOMIC_SLOT_SWAP"));
        Bag->ServerSwapSlots(0,1,Old0.InstanceId,Old1.InstanceId);
        Check(Bag->GetSlots()[1].InstanceId==Old0.InstanceId,TEXT("STALE_DRAG_REJECTED"));
        Bag->SwapSlots(0,1);auto Main=Bag->GetSlots()[9],Pocket=Bag->GetSlots()[10];Bag->SwapSlots(9,10);
        Check(Bag->GetSlots()[10].InstanceId==Main.InstanceId && Bag->GetEquippedItem()==Pocket.Item,TEXT("POCKET_MAIN_REAL_SWAP"));Bag->SwapSlots(9,10);
        Bag->ServerSwapSlots(-1,99,FGuid(),FGuid());Check(Bag->GetUsedSlots()==12,TEXT("INVALID_SWAP_REJECTED"));
    }
    if(Stage==5)
    {
        PC->BRLeave();Check(!PC->IsInventoryOpen() && !PC->IsMoveInputIgnored() && !PC->IsLookInputIgnored(),TEXT("ESC_CLOSES_BAG_NOT_ROOM"));Check(Bag->GetSanity()<SanityBefore,TEXT("SERVER_SAN_DRAINS"));
        for(int32 I=0;I<12;++I)if(Bag->GetSlots()[I].Item==EBRInventoryItem::AlmondWater){WaterSlot=I;break;}
        UsedId=Bag->GetSlots()[WaterSlot].InstanceId;Bag->SelectSlot(WaterSlot);WaterSlot=UBRInventoryComponent::MainHandSlot;SanityBefore=Bag->GetSanity();Bag->ServerUseSlot(WaterSlot,UsedId);Bag->ServerUseSlot(WaterSlot,UsedId);
        Check(Bag->GetUsedSlots()==11 && Bag->Count(EBRInventoryItem::AlmondWater)==5,TEXT("WATER_ONCE_ONLY"));Check(Bag->GetSanity()>SanityBefore && Bag->GetSanity()<=100,TEXT("SAN_RECOVERY_CLAMP"));
    }
    if(Stage==6)
    {Check(P->GetArmsAnimationComponent()->IsDrinking(),TEXT("DRINK_ANIMATION_STATE"));Capture(TEXT("04_drink"));}
    if(Stage==7)
    {Capture(TEXT("05_drink_end"));}
    if(Stage==8)
    {
        Check(!P->GetArmsAnimationComponent()->IsDrinking(),TEXT("DRINK_FINISHES"));
        Check(P->GrantAlmondWater(),TEXT("REFILL_FREED_SLOT"));const int32 N=Bag->GetUsedSlots();Bag->ServerUseSlot(WaterSlot,UsedId);Check(Bag->GetUsedSlots()==N,TEXT("STALE_TICKET_CANNOT_CONSUME_NEW_ITEM"));
        Bag->SelectSlot(WaterSlot);P->GetDownedComponent()->Down();Bag->UseSelected();Bag->DropSelected();Check(Bag->GetUsedSlots()==N,TEXT("DOWNED_USE_DROP_REJECTED"));
        auto* Other=GetWorld()->SpawnActor<ABRPlayerCharacter>(P->GetActorLocation()+FVector(150,0,0),FRotator::ZeroRotator);P->GetDownedComponent()->Revive(Other);Other->Destroy();Bag->SanityDrainPerSecond=0.1f;
    }
    if(Stage==9)
    {
        Bag->SelectSlot(WaterSlot);Bag->DropSelected();Check(Bag->GetUsedSlots()==11,TEXT("DROP_FREES_ONE_SLOT"));
        ABRSupplyPickup* Drop=nullptr;for(TActorIterator<ABRSupplyPickup> It(GetWorld());It;++It)if(!It->IsCollected() && It->SupplyType==EBRSupplyType::AlmondWater && FVector::Dist(It->GetActorLocation(),P->GetActorLocation())<250){Drop=*It;break;}
        Check(Drop!=nullptr,TEXT("DROPPED_WORLD_ITEM"));if(Drop){Drop->Interact_Implementation(P);Check(Drop->IsCollected() && Bag->GetUsedSlots()==12,TEXT("DROP_PICKUP_ROUNDTRIP"));}P->ToggleFlashlight();
    }
    if(Stage==10)
    {
        UE_LOG(LogTemp,Display,TEXT("BR_RELOAD before=%.2f batteries=%d"),P->GetBatteryCharge(),P->GetSpareBatteries());
        P->ReplaceBattery();Check(Bag->Count(EBRInventoryItem::Battery)==4 && P->GetSpareBatteries()==4 && P->GetBatteryCharge()==100,TEXT("RELOAD_CONSUMES_SLOT"));
        // Empty the held slot without mutating inventory data; empty hands use locomotion clips.
        int32 Empty=INDEX_NONE;for(int32 I=0;I<12;++I)if(Bag->GetSlots()[I].Item==EBRInventoryItem::Empty){Empty=I;break;}Bag->SelectSlot(Empty);
        PC->ToggleInventory();
    }
    if(Stage==11){Capture(TEXT("06_inventory_after_use"));}
    if(Stage==12)
    {PC->ToggleInventory();StepFixture(5);HandBefore=Arms->GetSocketTransform(TEXT("RightHand"),RTS_Component).GetLocation();}
    if(Stage>=13 && Stage<=16)UE_LOG(LogTemp,Display,TEXT("BR_STEP stage=%d pos=%s movement=%d floor=%d inputIgnored=%d"),Stage,*P->GetActorLocation().ToCompactString(),int32(P->GetCharacterMovement()->MovementMode),P->GetCharacterMovement()->CurrentFloor.IsWalkableFloor(),PC->IsMoveInputIgnored());
    if(Stage==13)
    {
        Check(P->GetActorLocation().X>-19700,TEXT("STEP_5_CM"));Check(P->GetArmsAnimationComponent()->GetAnimationState()==TEXT("Walk"),TEXT("WALK_ANIMATION_STATE"));
        Check(Arms->IsVisible(),TEXT("EMPTY_WALK_HANDS_VISIBLE"));StepFixture(20);
    }
    if(Stage==14){Check(P->GetActorLocation().X>-19700,TEXT("STEP_20_CM"));StepFixture(35);}
    if(Stage==15){Check(P->GetActorLocation().X>-19700,TEXT("STEP_35_CM"));StepFixture(60);}
    if(Stage==16)
    {Check(P->GetActorLocation().X<-19990 && P->GetActorLocation().Z<4150,TEXT("WALL_60_CM_BLOCKS"));Stand(FVector(3990,2160,3214),180);}
    if(Stage==17){Capture(TEXT("07_relaxed_arms"));Check(P->GetArmsAnimationComponent()->GetAnimationState()==TEXT("Idle"),TEXT("STOP_RETURNS_IDLE"));StepFixture(5);P->GetCharacterMovement()->MaxWalkSpeed=600;}
    if(Stage==18){Check(P->GetArmsAnimationComponent()->GetAnimationState()==TEXT("Run"),TEXT("RUN_ANIMATION_STATE"));StepFixture(5);P->GetCharacterMovement()->MaxWalkSpeed=350;P->Crouch();}
    if(Stage==19){Check(P->bIsCrouched && P->GetArmsAnimationComponent()->GetAnimationState()==TEXT("CrouchWalk"),TEXT("CROUCH_WALK_ANIMATION"));P->UnCrouch();Stand(FVector(3990,2160,3214),180);}
    if(Stage==20){Capture(TEXT("08_final_relaxed"));Check(Arms->IsVisible(),TEXT("EMPTY_FINAL_HANDS_VISIBLE"));Check(!P->bIsCrouched && P->GetArmsAnimationComponent()->GetAnimationState()==TEXT("Idle"),TEXT("UN_CROUCH_IDLE"));}
    if(Stage==21){UE_LOG(LogTemp,Display,TEXT("BR_INVENTORY_SMOKE result=%s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);FPlatformMisc::RequestExit(false);}
    ++Stage;
#endif
}

void ABRInventorySmokeProbe::TickArmsViews()
{
#if !UE_BUILD_SHIPPING
    const double Now=FPlatformTime::Seconds();
    if(!P)
    {
        P=Cast<ABRPlayerCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P)return;
        PC=Cast<ABRMenuPlayerController>(P->GetController());if(!PC){P=nullptr;return;}
        Bag=P->GetInventoryComponent();
        TArray<USkeletalMeshComponent*> Meshes;P->GetComponents(Meshes);
        for(auto* Mesh:Meshes)if(Mesh->GetFName()==TEXT("FirstPersonArms"))Arms=Mesh;
        for(TActorIterator<ABREntityCharacter> It(GetWorld());It;++It)
        {if(auto* AI=Cast<AAIController>(It->GetController())){AI->StopMovement();AI->UnPossess();}It->SetEntityState(EBREntityState::Returning);}
        Stand(FVector(3990,2160,3214),180);Started=Now;Stage=0;return;
    }
    if(Now-Started<2.5)return;
    const TCHAR* Names[]={TEXT("01_empty_forward"),TEXT("02_empty_look_down"),TEXT("03_empty_full_down"),TEXT("04_crouched_look_down"),TEXT("05_flashlight_forward"),TEXT("06_flashlight_down"),TEXT("07_stowed_look_down"),TEXT("08_almond_water"),TEXT("09_drink"),TEXT("10_chased_empty")};
    if(Stage>=UE_ARRAY_COUNT(Names))
    {
        UE_LOG(LogTemp,Display,TEXT("BR_ARMS_VIEWS result=%s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);
        FPlatformMisc::RequestExitWithStatus(false,Failures?2:0);return;
    }
    Check(Arms && Arms->IsVisible(),Names[Stage]);
    if(Stage==1 || Stage==2 || Stage==3 || Stage==6 || Stage==9)
    {
        Check(!Arms->IsBoneHiddenByName(TEXT("LeftShoulder")),TEXT("EMPTY_BOTH_ARMS"));
        for(const TCHAR* Bone:{TEXT("LeftHand"),TEXT("RightHand")})
        {
            FVector2D Screen;int32 Width=0,Height=0;PC->GetViewportSize(Width,Height);
            const bool InFront=PC->ProjectWorldLocationToScreen(Arms->GetSocketLocation(Bone),Screen);
            Check(InFront && Screen.X>=0 && Screen.X<Width && Screen.Y>=0 && Screen.Y<Height, TEXT("EMPTY_HAND_INSIDE_VIEW"));
            UE_LOG(LogTemp,Display,TEXT("BR_ARMS_VIEW stage=%d bone=%s screen=%s viewport=%dx%d front=%d local=%s"),Stage,Bone,*Screen.ToString(),Width,Height,InFront,*Arms->GetSocketTransform(Bone,RTS_Component).GetLocation().ToCompactString());
        }
    }
    Capture(Names[Stage]);
    ++Stage;Started=Now;
    // Apply the next pose after the screenshot has captured the current settled frame.
    FTimerHandle PoseTimer;
    GetWorld()->GetTimerManager().SetTimer(PoseTimer,FTimerDelegate::CreateWeakLambda(this,[this]()
    {
        if(Stage==1)PC->SetControlRotation(FRotator(-55,180,0));
        if(Stage==2)PC->SetControlRotation(FRotator(-80,180,0));
        if(Stage==3){P->Crouch();PC->SetControlRotation(FRotator(-55,180,0));}
        if(Stage==4){P->UnCrouch();PC->SetControlRotation(FRotator(0,180,0));P->GrantFlashlight();}
        if(Stage==5)PC->SetControlRotation(FRotator(-55,180,0));
        if(Stage==6)Bag->StowHeld();
        if(Stage==7){P->GrantAlmondWater();PC->SetControlRotation(FRotator(0,180,0));}
        if(Stage==8)Bag->UseSelected();
        if(Stage==9){Bag->StowHeld();P->GetStaminaComponent()->SetChasedBy(this,true);PC->SetControlRotation(FRotator(-80,180,0));}
    }),0.25f,false);
#endif
}
