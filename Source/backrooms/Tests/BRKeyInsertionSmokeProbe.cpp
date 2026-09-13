#include "Tests/BRKeyInsertionSmokeProbe.h"
#include "World/BRGarageKeyManager.h"
#include "World/BRGarageKeySocket.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRGarageExitDoor.h"
#include "World/BRExtractionZone.h"
#include "World/BRLootCabinet.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRPlayerState.h"
#include "Player/BRDownedComponent.h"
#include "Interaction/BRInteractionComponent.h"
#include "UI/BRMenuPlayerController.h"
#include "UI/BRGameHUDWidget.h"
#include "UI/BRRoundResultWidget.h"
#include "Core/BRGameState.h"
#include "AI/BREntityCharacter.h"
#include "AIController.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputKeyEventArgs.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
    TArray<ABRGarageKeyPickup*> SpawnKeys(UWorld* World,const ABRGarageKeyManager* Manager)
    {
        TArray<ABRGarageKeyPickup*> Keys;
        for(TActorIterator<ABRGarageKeyPickup> It(World);It;++It)
            if(It->GetOwner()==Manager)Keys.Add(*It);
        // Dynamic actor names differ between authority and clients; order the physical row instead.
        Keys.Sort([](const ABRGarageKeyPickup& A,const ABRGarageKeyPickup& B)
        {return FVector::DotProduct(A.GetActorLocation(),A.GetActorRightVector())<FVector::DotProduct(B.GetActorLocation(),B.GetActorRightVector());});
        return Keys;
    }
    void AttemptServerInteraction(ABRPlayerCharacter* Pawn,AActor* Target)
    {
        struct {AActor* TargetActor;} Params{Target};
        auto* Interaction=Pawn->GetInteractionComponent();
        Interaction->ProcessEvent(Interaction->FindFunctionChecked(TEXT("ServerInteract")),&Params);
    }
    void AimAt(ABRPlayerCharacter* Pawn,const FVector& Point)
    {
        Pawn->SetActorRotation(FRotator(0,(Point-Pawn->GetActorLocation()).Rotation().Yaw,0));
        FVector Eye;FRotator Rotation;Pawn->GetActorEyesViewPoint(Eye,Rotation);
        Pawn->GetController()->SetControlRotation((Point-Eye).Rotation());
    }
}
ABRKeyInsertionSmokeProbe::ABRKeyInsertionSmokeProbe()
{PrimaryActorTick.bCanEverTick=true;bReplicates=true;bAlwaysRelevant=true;}
void ABRKeyInsertionSmokeProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABRKeyInsertionSmokeProbe,Stage);DOREPLIFETIME(ABRKeyInsertionSmokeProbe,Subject);DOREPLIFETIME(ABRKeyInsertionSmokeProbe,Manager);}
void ABRKeyInsertionSmokeProbe::Check(bool OK,const TCHAR* Name)
{++Checks;if(!OK)++Failures;UE_LOG(LogTemp,Display,TEXT("BR_KEY_TEST case=%s result=%s stage=%d"),Name,OK?TEXT("PASS"):TEXT("FAIL"),Stage);}
void ABRKeyInsertionSmokeProbe::SetStage(int32 Value)
{Stage=Value;StageStarted=FPlatformTime::Seconds();ForceNetUpdate();}
void ABRKeyInsertionSmokeProbe::PlaceAtSocket(int32 Index,bool Both)
{
    auto* Socket=Manager->KeySockets[Index].Get();
    auto Place=[Socket](ABRPlayerCharacter* Pawn,float Side)
    {
        if(!Pawn)return;Pawn->GetCharacterMovement()->StopMovementImmediately();Pawn->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
        Pawn->SetActorLocation(Socket->GetActorTransform().TransformPositionNoScale(FVector(Side,115,-18)),false,nullptr,ETeleportType::TeleportPhysics);
        AimAt(Pawn,Socket->GetActorLocation());Pawn->ForceNetUpdate();
    };
    Place(Subject,Both && Other?-40:0);if(Both)Place(Other,40);
}
void ABRKeyInsertionSmokeProbe::CollectKeys(int32 Count)
{
    for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)It->SetDoorOpen(true);
    for(TActorIterator<ABRGarageKeyPickup> It(GetWorld());It && Manager->GetCollectedKeys()<Count;++It)
        if(It->IsPickupActive())It->Interact_Implementation(Subject);
}
void ABRKeyInsertionSmokeProbe::PlaceAtSpawnKey(int32 Index)
{
    const auto Keys=SpawnKeys(GetWorld(),Manager);
    if(!Keys.IsValidIndex(Index)){Check(false,TEXT("SPAWN_KEY_EXISTS"));return;}
    auto* Key=Keys[Index];
    Subject->GetCharacterMovement()->StopMovementImmediately();
    Subject->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    Subject->SetActorLocation(Key->GetActorLocation()-Key->GetActorForwardVector()*90.f+FVector(0,0,88),false,nullptr,ETeleportType::TeleportPhysics);
    AimAt(Subject,Key->GetActorLocation());Subject->ForceNetUpdate();
}
void ABRKeyInsertionSmokeProbe::Capture(const TCHAR* Name)
{
    if(Observed.Contains(Name))return;Observed.Add(Name);FString Path;
    if(FParse::Value(FCommandLine::Get(),TEXT("BRKeyCapture="),Path))FScreenshotRequest::RequestScreenshot(FPaths::Combine(Path,FString(Name)+TEXT(".png")),true,false);
}
void ABRKeyInsertionSmokeProbe::TickLocal()
{
#if !UE_BUILD_SHIPPING
    auto* PC=Cast<ABRMenuPlayerController>(GetGameInstance()->GetFirstLocalPlayerController());
    auto* Pawn=PC?Cast<ABRPlayerCharacter>(PC->GetPawn()):nullptr;
    if(!Pawn || !Subject || !Manager || Manager->KeySockets.Num()!=4)return;
    const bool IsSubject=Pawn==Subject;
    const bool SpawnKeyStage=Stage>=20 && Stage<=23;
    const auto TestKeys=SpawnKeys(GetWorld(),Manager);
    if(LocalStage!=Stage){LocalStage=Stage;LocalStageStarted=FPlatformTime::Seconds();bPressed=false;}
    const double Elapsed=FPlatformTime::Seconds()-LocalStageStarted;
    const int32 Index=Stage==7?1:Stage==8?2:Stage==9?3:0;
    if(Elapsed<0.7 && Stage>=1 && Stage<=9 && Stage!=2 && (IsSubject || Stage<=6))AimAt(Pawn,Manager->KeySockets[Index]->GetActorLocation());
    if(Elapsed<0.7 && Stage==10 && IsSubject)AimAt(Pawn,FVector(1318,7710,3265));
    if(IsSubject && (Stage==19 || SpawnKeyStage) && TestKeys.Num()==4)
        AimAt(Pawn,SpawnKeyStage?TestKeys[Stage-20]->GetActorLocation():(TestKeys[0]->GetActorLocation()+TestKeys[3]->GetActorLocation())*.5f);
    const bool PressStage=Stage==1 || Stage==3 || Stage==5 || Stage==6 || (IsSubject && ((Stage>=7 && Stage<=9) || SpawnKeyStage));
    if(PressStage && Elapsed>0.8 && !bPressed)
    {
        bPressed=true;auto* Focus=Pawn->GetInteractionComponent()->FindFocusedInteractable();
        AActor* Expected=SpawnKeyStage?(TestKeys.IsValidIndex(Stage-20)?static_cast<AActor*>(TestKeys[Stage-20]):nullptr):Manager->KeySockets[Index].Get();
        Check(Expected && Focus==Expected,SpawnKeyStage?TEXT("REAL_E_AIMS_AT_SPAWN_KEY"):TEXT("REAL_E_AIMS_AT_SOCKET"));
        if(Focus!=Expected)UE_LOG(LogTemp,Display,TEXT("BR_KEY_TRACE expected=%s actual=%s pawn=%s rotation=%s"),*GetNameSafe(Expected),*GetNameSafe(Focus),*Pawn->GetActorLocation().ToCompactString(),*PC->GetControlRotation().ToCompactString());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E,IE_Pressed,1));
    }
    if(bPressed && Elapsed>1.0)PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::E,IE_Released,0));
    auto Observe=[this](bool OK,FName Name)
    {if(OK && !Observed.Contains(Name)){Observed.Add(Name);UE_LOG(LogTemp,Display,TEXT("BR_KEY_CLIENT case=%s result=PASS"),*Name.ToString());}};
    if(Stage==19)
    {
        bool AllActive=TestKeys.Num()==4;
        for(auto* Key:TestKeys)AllActive &= Key->IsPickupActive() && !Key->IsHidden();
        Observe(AllActive && Manager->GetRequiredKeys()==4 && Manager->GetCollectedKeys()==0,TEXT("FOUR_SPAWN_KEYS_VISIBLE"));
        if(AllActive && Elapsed>1.2)Capture(TEXT("00_spawn_test_keys"));
    }
    Observe(Stage==1 && Manager->GetInsertedKeys()==0 && !Manager->ExitDoor->IsOpen() && !Manager->KeySockets[0]->IsInsertedKeyVisible(),TEXT("FRESH_ROUND_EMPTY"));
    Observe(Stage==4 && Manager->GetCollectedKeys()==4 && Manager->GetInsertedKeys()==0 && !Manager->ExitDoor->IsOpen() && !PC->IsResultScreenVisible(),TEXT("FOUR_COLLECTED_STILL_CLOSED"));
    Observe(Stage>=6 && Stage<=7 && Manager->GetInsertedKeys()==1 && Manager->KeySockets[0]->IsInsertedKeyVisible() && !Manager->KeySockets[1]->IsInsertedKeyVisible(),TEXT("FIRST_INSERT_REPLICATED_ONCE"));
    Observe(Stage==9 && Manager->GetInsertedKeys()==3 && !Manager->ExitDoor->IsOpen(),TEXT("THREE_INSERTED_STILL_CLOSED"));
    bool AllVisible=true;for(const auto& Socket:Manager->KeySockets)AllVisible &= Socket && Socket->IsInsertedKeyVisible();
    Observe(Stage==10 && Manager->GetInsertedKeys()==4 && AllVisible && Manager->ExitDoor->IsPassageOpen() && !PC->IsResultScreenVisible(),TEXT("FOUR_INSERTED_DOOR_OPEN"));
    if(Elapsed>1.2)
    {
        const FName PromptCase=Stage==1?TEXT("EMPTY_SOCKET_HINT"):Stage==4?TEXT("INSERT_E_HINT"):Stage==6?TEXT("INSERTED_HINT"):TEXT("");
        if(!PromptCase.IsNone() && !Observed.Contains(PromptCase))
        {
            Observed.Add(PromptCase);TArray<UUserWidget*> HUDs;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,HUDs,UBRGameHUDWidget::StaticClass(),true);
            auto* HUD=HUDs.Num()==1?Cast<UBRGameHUDWidget>(HUDs[0]):nullptr;
            auto* Hint=HUD?Cast<UTextBlock>(HUD->WidgetTree->FindWidget(TEXT("InteractionHint"))):nullptr;
            const TCHAR* Expected=Stage==1?TEXT("需要集齐4把钥匙"):Stage==4?TEXT("按E插入钥匙"):TEXT("钥匙已插入");
            Check(Hint && Hint->GetText().ToString().Contains(Expected) && HUD->IsPickupPromptVisible() && !HUD->ShouldDrawInteractionRing(),*PromptCase.ToString());
        }
        if(Stage==1)Capture(TEXT("01_empty_socket"));
        if(Stage==4)Capture(TEXT("02_ready_to_insert"));
        if(Stage==6)Capture(TEXT("03_inserted_socket"));
        if(Stage==10)Capture(TEXT("04_central_door_open"));
    }
    if(Stage==11 && IsSubject && GetWorld()->GetGameState<ABRGameState>()->GetLevelPhase()!=EBRLevelPhase::Completed)
        Pawn->AddMovementInput(FVector(0,-1,0));
    if(PC->IsResultScreenVisible() && !bResultObserved)
    {
        TArray<UUserWidget*> Results;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Results,UBRRoundResultWidget::StaticClass(),true);
        auto* Title=Results.Num()==1?Cast<UTextBlock>(Results[0]->WidgetTree->FindWidget(TEXT("ResultTitle"))):nullptr;
        if(!Title || Title->GetCachedGeometry().GetLocalSize().X<=0)return;
        bResultObserved=true;
        const bool OK=Title && Title->GetText().ToString()==TEXT("逃生成功") && PC->IsMoveInputIgnored() && PC->IsLookInputIgnored();
        Check(OK,TEXT("SUCCESS_PAGE_AND_INPUT_LOCK"));Observe(OK,TEXT("SUCCESS_PAGE"));Capture(TEXT("05_escape_result"));
    }
    if(Stage==13 && GetNetMode()==NM_Standalone && Elapsed>1.0 && !bPressed)
    {
        bPressed=true;TArray<UUserWidget*> Results;UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this,Results,UBRRoundResultWidget::StaticClass(),true);
        auto* Button=Results.Num()==1?Cast<UButton>(Results[0]->WidgetTree->FindWidget(TEXT("ReturnMenuButton"))):nullptr;
        Check(Button!=nullptr,TEXT("RESULT_RETURN_BUTTON"));if(Button)Button->OnClicked.Broadcast();
    }
#endif
}
void ABRKeyInsertionSmokeProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if(GetNetMode()!=NM_DedicatedServer)TickLocal();
    if(!HasAuthority() || Stage==13 || Stage==99)return;
    const double Now=FPlatformTime::Seconds();if(!Started)Started=Now;
    if(Now-Started>100){Check(false,TEXT("TIMEOUT"));SetStage(99);FPlatformMisc::RequestExitWithStatus(false,2);return;}
    if(Stage==0)
    {
        if(Now-Started<3)return;
        for(TActorIterator<ABRPlayerCharacter> It(GetWorld());It;++It)
            if(auto* PS=It->GetPlayerState<ABRPlayerState>()){if(GetNetMode()==NM_Standalone || PS->IsRoomOwner())Subject=*It;else Other=*It;}
        if(!Subject || (GetNetMode()==NM_DedicatedServer && !Other))return;
        for(TActorIterator<ABRGarageKeyManager> It(GetWorld());It;++It){Manager=*It;break;}
        for(TActorIterator<ABRExtractionZone> It(GetWorld());It;++It){Zone=*It;break;}
        Check(Manager && Manager->KeySockets.Num()==4 && Manager->ExitDoor && Zone && Zone->RequiredExitDoor==Manager->ExitDoor,TEXT("SCENE_BINDINGS"));
        if(!Manager || Manager->KeySockets.Num()!=4 || !Manager->ExitDoor || !Zone){SetStage(99);FPlatformMisc::RequestExitWithStatus(false,2);return;}
        for(TActorIterator<ABREntityCharacter> It(GetWorld());It;++It)if(auto* AI=Cast<AAIController>(It->GetController())){AI->StopMovement();AI->UnPossess();}
        // Let cabinet opening animations expose their loot before the collection stages.
        for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)It->SetDoorOpen(true);
        Check(Manager->GetCollectedKeys()==0 && Manager->GetInsertedKeys()==0 && !Manager->ExitDoor->IsOpen(),TEXT("FRESH_ZERO_COUNTS_CLOSED_DOOR"));
        for(const auto& Socket:Manager->KeySockets)Check(Socket && !Socket->IsInserted() && !Socket->IsInsertedKeyVisible(),TEXT("SAVED_SOCKET_STARTS_EMPTY"));
        Check(!Manager->ExitDoor->OpenAfterKeysInserted() && !Manager->ExitDoor->CanInteract_Implementation(Subject),TEXT("EXIT_CANNOT_MANUALLY_OPEN"));
        if(FParse::Param(FCommandLine::Get(),TEXT("BRSpawnKeysSmoke")))
        {
            const auto Keys=SpawnKeys(GetWorld(),Manager);
            Check(Keys.Num()==4,TEXT("FOUR_EXTRA_SPAWN_KEYS"));
            int32 WorldKeys=0;
            for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)if(It->GetKey())++WorldKeys;
            Check(WorldKeys==4,TEXT("FOUR_ORIGINAL_CABINET_KEYS_REMAIN"));
            Check(Manager->GetRequiredKeys()==4 && GetWorld()->GetGameState<ABRGameState>()->GetTotalObjectives()==4,TEXT("GOAL_REMAINS_FOUR"));
            SetStage(19);return;
        }
        PlaceAtSocket(0,true);SetStage(1);return;
    }
    const double Elapsed=Now-StageStarted;auto* State=GetWorld()->GetGameState<ABRGameState>();
    if(Elapsed<(Stage==12?1.0:2.5))return;
    if(Stage==19){PlaceAtSocket(0,true);SetStage(1);return;}
    if(Stage>=20 && Stage<=23)
    {
        const int32 Collected=Stage-19;
        Check(Manager->GetCollectedKeys()==Collected,TEXT("SPAWN_KEY_COLLECTED_WITH_REAL_E"));
        if(Stage<22){PlaceAtSpawnKey(Collected);SetStage(Stage+1);}
        else {PlaceAtSocket(0,true);SetStage(Stage==22?3:4);}
        return;
    }
    if(Stage==1)
    {Check(Manager->GetInsertedKeys()==0,TEXT("ZERO_KEYS_REJECT_E"));Subject->SetActorLocation(Zone->GetActorLocation());SetStage(2);return;}
    if(Stage==2)
    {
        Check(State->GetLevelPhase()==EBRLevelPhase::Exploring,TEXT("EARLY_EXIT_ZONE_NO_RESULT"));
        if(FParse::Param(FCommandLine::Get(),TEXT("BRSpawnKeysSmoke"))){PlaceAtSpawnKey(0);SetStage(20);return;}
        CollectKeys(3);PlaceAtSocket(0,true);SetStage(3);return;
    }
    if(Stage==3)
    {
        Check(Manager->GetCollectedKeys()==3 && Manager->GetInsertedKeys()==0,TEXT("THREE_KEYS_REJECT_E"));
        if(FParse::Param(FCommandLine::Get(),TEXT("BRSpawnKeysSmoke"))){PlaceAtSpawnKey(3);SetStage(23);return;}
        CollectKeys(4);SetStage(4);return;
    }
    if(Stage==4)
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("BRSpawnKeysSmoke")))
        {
            ABRGarageKeyPickup* Extra=nullptr;
            for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)if(It->GetKey()){Extra=It->GetKey();break;}
            if(Extra)Extra->Interact_Implementation(Subject);
            Check(Extra && Extra->IsCollected() && Manager->GetCollectedKeys()==4 && State->GetTotalObjectives()==4,TEXT("EXTRA_WORLD_KEY_KEEPS_GOAL_AT_FOUR"));
        }
        Check(Manager->GetCollectedKeys()==4 && Manager->GetInsertedKeys()==0 && State->GetCompletedObjectives()==0 && !Manager->ExitDoor->IsOpen(),TEXT("COLLECTION_DOES_NOT_OPEN_OR_FINISH"));
        const FVector Saved=Subject->GetActorLocation();Subject->SetActorLocation(Saved+FVector(0,800,0));AttemptServerInteraction(Subject,Manager->KeySockets[0]);
        Check(Manager->GetInsertedKeys()==0,TEXT("SERVER_REJECTS_OUT_OF_RANGE"));Subject->SetActorLocation(Saved);
        Subject->GetDownedComponent()->Down();AttemptServerInteraction(Subject,Manager->KeySockets[0]);
        Check(Manager->GetInsertedKeys()==0,TEXT("SERVER_REJECTS_DOWNED"));
        ABRPlayerCharacter* Reviver=Other;
        if(!Reviver)
        {
            FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
            Reviver=GetWorld()->SpawnActor<ABRPlayerCharacter>(Saved+FVector(80,0,0),FRotator::ZeroRotator,Params);
        }
        Subject->GetDownedComponent()->Revive(Reviver);if(!Other && Reviver)Reviver->Destroy();
        Check(!Subject->GetDownedComponent()->IsDowned(),TEXT("FIXTURE_REVIVES_BEFORE_INSERT"));PlaceAtSocket(0,true);
        auto* Obstruction=GetWorld()->SpawnActor<AStaticMeshActor>((Subject->GetActorLocation()+Manager->KeySockets[0]->GetActorLocation())*.5f,FRotator::ZeroRotator);
        Obstruction->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);Obstruction->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Obstruction->SetActorScale3D(FVector(.5,.5,2));
        AttemptServerInteraction(Subject,Manager->KeySockets[0]);Check(Manager->GetInsertedKeys()==0,TEXT("SERVER_REJECTS_OCCLUDED"));Obstruction->Destroy();
        FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(BRKeyDoorProbe),false,Subject);
        GetWorld()->SweepSingleByChannel(Hit,FVector(1318,8050,3210),FVector(1318,7740,3210),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,88),Params);
        Check(Hit.bBlockingHit,TEXT("CLOSED_DOOR_BLOCKS_CAPSULE"));
        SetStage(5);return;
    }
    if(Stage==5){Check(Manager->GetInsertedKeys()==1 && Manager->KeySockets[0]->IsInsertedKeyVisible(),TEXT("SIMULTANEOUS_E_INSERTS_ONCE"));SetStage(6);return;}
    if(Stage==6){Check(Manager->GetInsertedKeys()==1,TEXT("REPEATED_E_DOES_NOT_SPEND_AGAIN"));PlaceAtSocket(1);SetStage(7);return;}
    if(Stage==7){Check(Manager->GetInsertedKeys()==2 && !Manager->ExitDoor->IsOpen(),TEXT("SECOND_SOCKET_STILL_CLOSED"));PlaceAtSocket(2);SetStage(8);return;}
    if(Stage==8){Check(Manager->GetInsertedKeys()==3 && !Manager->ExitDoor->IsOpen(),TEXT("THIRD_SOCKET_STILL_CLOSED"));PlaceAtSocket(3);SetStage(9);return;}
    if(Stage==9)
    {
        Check(Manager->AreAllKeysInserted() && Manager->ExitDoor->IsPassageOpen() && State->GetLevelPhase()==EBRLevelPhase::ExtractionReady,TEXT("FOURTH_SOCKET_OPENS_DOOR"));
        Subject->GetCharacterMovement()->SetMovementMode(MOVE_Walking);Subject->SetActorLocation(FVector(1318,8050,3210));AimAt(Subject,FVector(1318,7710,3265));
        if(Other)Other->SetActorLocation(FVector(1550,8200,3210));SetStage(10);return;
    }
    if(Stage==10){Check(State->GetLevelPhase()==EBRLevelPhase::ExtractionReady,TEXT("STANDING_OUTSIDE_NO_RESULT"));SetStage(11);return;}
    if(Stage==11)
    {
        Check(State->GetLevelPhase()==EBRLevelPhase::Completed && Subject->GetActorLocation().Y<7850,TEXT("WALK_THROUGH_DOOR_STARTS_RESULT"));
        UE_LOG(LogTemp,Display,TEXT("BR_KEY_WALK position=%s"),*Subject->GetActorLocation().ToCompactString());
        Check(!Other || Other->GetActorLocation().Y>7900,TEXT("ONE_ENTERING_PLAYER_EXTRACTS_TEAM"));SetStage(12);return;
    }
    if(Stage==12)
    {
        if(GetNetMode()==NM_Standalone)Check(bResultObserved,TEXT("RESULT_ACTUALLY_LAID_OUT"));
        UE_LOG(LogTemp,Display,TEXT("BR_KEY_TEST result=%s checks=%d failures=%d"),Failures?TEXT("FAIL"):TEXT("PASS"),Checks,Failures);SetStage(13);
    }
#endif
}
