#include "UI/BRMenuPlayerController.h"
#include "Interaction/BRInteractionComponent.h"
#include "UI/BRMenuWidget.h"
#include "UI/BRInventoryWidget.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRDownedComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UI/BRGameHUDWidget.h"
#include "UI/BRRoundResultWidget.h"
#include "Player/BRPlayerCharacter.h"
#include "World/BRGarageKeyManager.h"
#include "World/BRGarageDoor.h"
#include "World/BRLootCabinet.h"
#include "World/BRSupplyPickup.h"
#include "EngineUtils.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Online/BRRoomSettings.h"
#include "Core/BRGameState.h"
#include "Player/BRPlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "InputCoreTypes.h"
#include "UnrealClient.h"
#include "Camera/CameraActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"

void ABRMenuPlayerController::BeginPlay()
{
    Super::BeginPlay(); TestStarted = FPlatformTime::Seconds();
#if !UE_BUILD_SHIPPING
    FParse::Value(FCommandLine::Get(), TEXT("BRTestRole="), TestRole);
#endif
    if (IsLocalController()) UpdatePresentation();
#if !UE_BUILD_SHIPPING
    if (IsLocalController() && FParse::Param(FCommandLine::Get(), TEXT("BRParkingPreview")))
    {
        auto* Camera = GetWorld()->SpawnActor<ACameraActor>(FVector(3606.28,2161.25,3303.45),FRotator(-11.92635,-220.402458,0));
        SetViewTarget(Camera);
    }
#endif
}
void ABRMenuPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();
    InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &ThisClass::BRLeave);
    InputComponent->BindKey(EKeys::Tab,IE_Pressed,this,&ThisClass::ToggleInventory);
    InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&ThisClass::UseHeldItem);
    InputComponent->BindKey(EKeys::G,IE_Pressed,this,&ThisClass::DropHeldItem);
    const FKey Keys[]={EKeys::One,EKeys::Two};
    for(int32 I=0;I<2;++I){FInputKeyBinding B(FInputChord(Keys[I]),IE_Pressed);B.KeyDelegate.GetDelegateForManualSet().BindLambda([this,I](){QuickSlot(I);});InputComponent->KeyBindings.Add(B);}
}
void ABRMenuPlayerController::BRLeave()
{
    if(Inventory){CloseInventory();return;}
    if (IsLocalController()) GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->LeaveRoom();
}
void ABRMenuPlayerController::ServerSetLobbyReady_Implementation(bool bReady)
{
    if (!GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->RequestReady(GetPlayerState<ABRPlayerState>(), bReady))
        ClientMenuMessage(TEXT("当前阶段不能更改准备状态。"));
}
void ABRMenuPlayerController::ServerStartRoom_Implementation()
{
    if (!GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->RequestStart(GetPlayerState<ABRPlayerState>()))
        ClientMenuMessage(TEXT("仅房主可以开始；请等待所有玩家准备且连接完成。"));
}
void ABRMenuPlayerController::ClientMenuMessage_Implementation(const FString& Message)
{
    if (Menu) Menu->ShowStatus(Message);
    UE_LOG(LogTemp, Display, TEXT("BR_MENU message=%s"), *Message);
#if !UE_BUILD_SHIPPING
    if (FParse::Param(FCommandLine::Get(),TEXT("BRTestIllegalStart")) && Message.StartsWith(TEXT("仅房主")))
        UE_LOG(LogTemp,Display,TEXT("BR_PROBE case=NON_OWNER_START result=PASS"));
#endif
}
void ABRMenuPlayerController::UpdatePresentation()
{
    // Seamless travel can leave the retiring controller locally flagged after its Player moved.
    if (!IsLocalController() || !GetLocalPlayer()) return;
    const FString Map = GetWorld()->GetMapName();
    const bool bLobby = Map.EndsWith(TEXT("L_Lobby"));
    const bool bMenu = bLobby || Map.EndsWith(TEXT("L_MainMenu"));
    if (Map == PresentedWorld) return;
    CloseInventory();
    if(RoundResult){RoundResult->RemoveFromParent();RoundResult=nullptr;}
    if(bRoundInputLocked){SetIgnoreMoveInput(false);SetIgnoreLookInput(false);bRoundInputLocked=false;}
    PresentedWorld = Map;
    // A client controller can receive PlayerTick before BeginPlay during login.
    // Start capture timing on presentation, not an uninitialized controller clock.
    TestStarted = FPlatformTime::Seconds(); bScreenshotRequested = false;
    if (Menu) { Menu->RemoveFromParent(); Menu = nullptr; }
    if (GameHUD) { GameHUD->RemoveFromParent(); GameHUD = nullptr; }
    if (bMenu)
    {
        bShowMouseCursor = true; SetInputMode(FInputModeUIOnly());
        if (FApp::CanEverRender())
        {
            const auto Class = LoadClass<UBRMenuWidget>(nullptr, TEXT("/Game/UI/Menu/Widgets/WBP_MainMenu.WBP_MainMenu_C"));
            if (Class) { Menu = CreateWidget<UBRMenuWidget>(this, Class);
                if (!Menu) { PresentedWorld.Empty(); return; }
                Menu->AddToViewport(); Menu->SetPage(bLobby ? 2 : 0);
#if !UE_BUILD_SHIPPING
                int32 PreviewPage = -1;
                if (FParse::Value(FCommandLine::Get(), TEXT("BRMenuPage="), PreviewPage)) Menu->SetPage(PreviewPage);
#endif
            }
        }
    }
    else
    {
        bShowMouseCursor = false; SetInputMode(FInputModeGameOnly());
        if (FApp::CanEverRender() && Map.EndsWith(TEXT("L_MiddleFloor_Dark")))
        {
            auto Class = LoadClass<UBRGameHUDWidget>(nullptr,TEXT("/Game/UI/Menu/Widgets/WBP_GameHUD.WBP_GameHUD_C"));
            if (Class) {
                GameHUD=CreateWidget<UBRGameHUDWidget>(this,Class);
                if (!GameHUD) { PresentedWorld.Empty();return; }
                GameHUD->AddToViewport();GameHUD->SetVisibility(ESlateVisibility::HitTestInvisible);
            }
        }
    }
    UE_LOG(LogTemp, Display, TEXT("BR_PRESENTATION map=%s menu=%d"), *Map, bMenu);
}
void ABRMenuPlayerController::PlayerTick(float DeltaTime)
{
    Super::PlayerTick(DeltaTime);
    if (!IsLocalController() || !GetLocalPlayer() || FPlatformTime::Seconds() < NextUpdate) return;
    NextUpdate = FPlatformTime::Seconds() + 0.25; UpdatePresentation();
    UpdateRoundResult();
    if(Inventory){auto* P=Cast<ABRPlayerCharacter>(GetPawn());if(!P || P->GetDownedComponent()->IsDowned() || P->GetCurrentHideSpot())CloseInventory();}
    if (!TestRole.IsEmpty()) RunSmokeDriver();
#if !UE_BUILD_SHIPPING
    FString Capture;
    if(FParse::Param(FCommandLine::Get(),TEXT("BRKeyResultReturn")) && GetWorld()->GetMapName().EndsWith(TEXT("L_MainMenu")) && FPlatformTime::Seconds()-TestStarted>1)
    {
        const bool OK=Menu && !RoundResult && !IsMoveInputIgnored() && !IsLookInputIgnored();
        UE_LOG(LogTemp,Display,TEXT("BR_KEY_CLIENT case=RETURN_MAIN_MENU result=%s"),OK?TEXT("PASS"):TEXT("FAIL"));FPlatformMisc::RequestExitWithStatus(false,OK?0:2);return;
    }
    if (FApp::CanEverRender() && FParse::Value(FCommandLine::Get(), TEXT("BRMenuCapture="), Capture))
    {
        const double Elapsed = FPlatformTime::Seconds() - TestStarted;
        if (Elapsed > 12 && !bScreenshotRequested) { bScreenshotRequested = true; FScreenshotRequest::RequestScreenshot(Capture,true,false); }
        if (Elapsed > 16) FPlatformMisc::RequestExit(false);
    }
#endif
}
void ABRMenuPlayerController::RunSmokeDriver()
{
#if !UE_BUILD_SHIPPING
    auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>();
    bool& bTestCreate = D->bSmokeCreate; bool& bTestJoin = D->bSmokeJoin;
    bool& bTestStart = D->bSmokeStart; bool& bTestSawGame = D->bSmokeSawGame;
    if (D->SmokeStarted == 0) D->SmokeStarted = FPlatformTime::Seconds();
    auto* GS = GetWorld()->GetGameState<ABRGameState>(); auto* PS = GetPlayerState<ABRPlayerState>();
    const FString Map = GetWorld()->GetMapName();
    const bool bUseUI=FParse::Param(FCommandLine::Get(),TEXT("BRTestUseUI"));
    auto Press=[this](const TCHAR* Name)
    {
        if (!Menu) return false;
        auto* Button=Cast<UButton>(Menu->WidgetTree->FindWidget(Name));
        if (!Button || !Button->GetIsEnabled()) return false;
        Button->OnClicked.Broadcast();UE_LOG(LogTemp,Display,TEXT("BR_UI_FLOW button=%s result=CLICKED"),Name);return true;
    };
    if (TestRole.StartsWith(TEXT("probe:")))
    {
        const FString Expected=TestRole.RightChop(6);
        if (Expected==TEXT("VERSION")) GetMutableDefault<UBRRoomSettings>()->BuildId=TEXT("intentionally-wrong-test-build");
        if (!D->bBusy)
        {
            if (D->LastReply.Code==Expected || (Expected==TEXT("EMPTY") && D->LastReply.bSuccess && !D->LastReply.Room.RoomId.IsValid()))
            { UE_LOG(LogTemp,Display,TEXT("BR_PROBE case=%s result=PASS"),*Expected); FPlatformMisc::RequestExit(false); return; }
            if (D->LastReply.bSuccess && D->LastReply.Room.RoomId.IsValid() && !bTestJoin)
            {
                if (Expected==TEXT("ROOM_EXISTS")) { bTestJoin=true; D->CreateRoom(TEXT("Duplicate"),TEXT("Other"),4); }
                else if ((Expected==TEXT("FULL") && D->LastReply.Room.CurrentPlayers==4) ||
                    (Expected==TEXT("IN_PROGRESS") && D->LastReply.Room.Phase==EBRRoomPhase::InGame))
                { bTestJoin=true; D->JoinRoom(D->LastReply.Room.RoomId,TEXT("Late")); }
                else D->QueryRooms();
            }
            else D->QueryRooms();
        }
        if (FPlatformTime::Seconds()-D->SmokeStarted>90)
        { UE_LOG(LogTemp,Error,TEXT("BR_PROBE case=%s result=TIMEOUT"),*Expected);FPlatformMisc::RequestExitWithStatus(false,2); }
        return;
    }
    if (Map.EndsWith(TEXT("L_MainMenu")) && D->bSmokeWasInLobby)
    {
        if (D->bSmokeDeliberateLeave || FParse::Param(FCommandLine::Get(),TEXT("BRExpectDisconnect")))
        { UE_LOG(LogTemp,Display,TEXT("BR_FAULT case=%s result=PASS"),D->bSmokeDeliberateLeave?TEXT("LEAVE"):TEXT("DISCONNECT"));FPlatformMisc::RequestExit(false);return; }
    }
    if (Map.EndsWith(TEXT("L_MainMenu")) && !D->bBusy)
    {
        if (TestRole == TEXT("host") && !bTestCreate)
        {
            if (bUseUI)
            {
                if (Menu && Press(TEXT("PlayButton")))
                {
                    CastChecked<UEditableTextBox>(Menu->WidgetTree->FindWidget(TEXT("NicknameBox")))->SetText(FText::FromString(TEXT("房主测试")));
                    CastChecked<UEditableTextBox>(Menu->WidgetTree->FindWidget(TEXT("RoomNameBox")))->SetText(FText::FromString(TEXT("停车场联机测试")));
                    auto* Capacity=CastChecked<UComboBoxString>(Menu->WidgetTree->FindWidget(TEXT("CapacityBox")));
                    if (Capacity->GetOptionCount()==4 && Capacity->GetSelectedOption()==TEXT("4")) bTestCreate=Press(TEXT("StoryButton"));
                }
            }
            else { bTestCreate = true; D->CreateRoom(TEXT("Parking integration"), TEXT("Host"), 4); }
        }
        else if (TestRole != TEXT("host") && !bTestJoin)
        {
            if (D->LastReply.bSuccess && D->LastReply.Room.bJoinable)
            {
                if (bUseUI) bTestJoin=Press(TEXT("JoinButton"));
                else { bTestJoin = true; D->JoinRoom(D->LastReply.Room.RoomId, TestRole); }
            }
            else if (bUseUI)
            {
                if (Menu && !D->bSmokeBrowseOpened)
                {
                    CastChecked<UEditableTextBox>(Menu->WidgetTree->FindWidget(TEXT("NicknameBox")))->SetText(FText::FromString(TEXT("队员测试")));
                    D->bSmokeBrowseOpened=Press(TEXT("BrowseButton"));
                }
            }
            else D->QueryRooms();
        }
    }
    if (Map.EndsWith(TEXT("L_Lobby")) && GS && PS && PS->GetRoomPlayerId().IsValid())
    {
        D->bSmokeWasInLobby=true;
        if (D->SmokeLobbyEntered==0) D->SmokeLobbyEntered=FPlatformTime::Seconds();
        float LeaveAfter=0;
        if (FParse::Value(FCommandLine::Get(),TEXT("BRTestLeaveAfter="),LeaveAfter) && FPlatformTime::Seconds()-D->SmokeLobbyEntered>LeaveAfter)
        { D->bSmokeDeliberateLeave=true;D->LeaveRoom();return; }
        if (FParse::Param(FCommandLine::Get(),TEXT("BRExpectOwnerTransfer")) && PS->IsRoomOwner() && GS->RoomInfo.CurrentPlayers==1)
        { UE_LOG(LogTemp,Display,TEXT("BR_FAULT case=OWNER_TRANSFER result=PASS"));FPlatformMisc::RequestExit(false);return; }
        if (bTestSawGame)
        {
            ++D->SmokeRoundsCompleted; bTestSawGame=false;bTestStart=false;
            UE_LOG(LogTemp, Display, TEXT("BR_NETTEST role=%s result=ROUND_TRIP_PASS round=%d"), *TestRole,D->SmokeRoundsCompleted);
            int32 Rounds=1;FParse::Value(FCommandLine::Get(),TEXT("BRTestRounds="),Rounds);
            if (D->SmokeRoundsCompleted>=Rounds) { FPlatformMisc::RequestExit(false); return; }
        }
        if (!PS->IsReady()) { if (bUseUI) Press(TEXT("ReadyButton")); else ServerSetLobbyReady(true); }
        if (!PS->IsRoomOwner() && !D->bSmokeIllegalStart && FParse::Param(FCommandLine::Get(),TEXT("BRTestIllegalStart")))
        { D->bSmokeIllegalStart=true;ServerStartRoom(); }
        int32 Expected = 2; FParse::Value(FCommandLine::Get(), TEXT("BRExpectedPlayers="), Expected);
        bool bReady = GS->RoomInfo.CurrentPlayers >= Expected && GS->PlayerArray.Num() >= Expected;
        for (APlayerState* Base : GS->PlayerArray) if (auto* Other = Cast<ABRPlayerState>(Base)) bReady &= Other->IsReady();
        if (PS->IsRoomOwner() && bReady && !bTestStart && !FParse::Param(FCommandLine::Get(),TEXT("BRTestNoStart")))
        { if (bUseUI) bTestStart=Press(TEXT("StartButton")); else { bTestStart = true; ServerStartRoom(); } }
    }
    if (Map.EndsWith(TEXT("L_MiddleFloor_Dark")) && GetPawn() && GS && GS->RoomInfo.Phase == EBRRoomPhase::InGame && !bTestSawGame)
    {
        bTestSawGame = true;ItemSmokeObserved.Reset();ItemSmokeDoor.Reset();
        D->bSmokeHiddenObserved=false;D->bSmokeExitObserved=false;D->bSmokeKeysObserved=false;D->bSmokeDoorObserved=false;
        D->bSmokeRemoteHidden=false;D->bSmokeRemoteExit=false;
        UE_LOG(LogTemp, Display, TEXT("BR_NETTEST role=%s result=ENTERED_GAME players=%d location=%s"), *TestRole, GS->RoomInfo.CurrentPlayers, *GetPawn()->GetActorLocation().ToCompactString());
    }
    if (Map.EndsWith(TEXT("L_MiddleFloor_Dark")))
    {
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld());It;++It)
            if (auto* Other=It->GetPlayerState<ABRPlayerState>()) if (Other->IsRoomOwner())
            {
                if (It->GetCurrentHideSpot() && It->IsHidden() && !D->bSmokeRemoteHidden)
                { D->bSmokeRemoteHidden=true;UE_LOG(LogTemp,Display,TEXT("BR_CLIENT_SYNC role=%s case=OWNER_HIDDEN_ON result=PASS"),*TestRole); }
                if (D->bSmokeRemoteHidden && !It->GetCurrentHideSpot() && !It->IsHidden() && !D->bSmokeRemoteExit)
                { D->bSmokeRemoteExit=true;UE_LOG(LogTemp,Display,TEXT("BR_CLIENT_SYNC role=%s case=OWNER_HIDDEN_OFF result=PASS"),*TestRole); }
            }
        if (auto* PlayerCharacter=Cast<ABRPlayerCharacter>(GetPawn()))
        {
            if (PlayerCharacter->GetCurrentHideSpot() && !D->bSmokeHiddenObserved)
            { D->bSmokeHiddenObserved=true;UE_LOG(LogTemp,Display,TEXT("BR_CLIENT_SYNC role=%s case=HIDDEN_ON result=PASS"),*TestRole); }
            if (D->bSmokeHiddenObserved && !PlayerCharacter->GetCurrentHideSpot() && !D->bSmokeExitObserved)
            { D->bSmokeExitObserved=true;UE_LOG(LogTemp,Display,TEXT("BR_CLIENT_SYNC role=%s case=HIDDEN_OFF result=PASS"),*TestRole); }
        }
        for (TActorIterator<ABRGarageKeyManager> It(GetWorld());It;++It) if (It->GetCollectedKeys()==1 && !D->bSmokeKeysObserved)
        { D->bSmokeKeysObserved=true;UE_LOG(LogTemp,Display,TEXT("BR_CLIENT_SYNC role=%s case=KEY_COUNT result=PASS"),*TestRole); }
        for (TActorIterator<ABRGarageDoor> It(GetWorld());It;++It) if (It->IsOpen() && !D->bSmokeDoorObserved)
        { D->bSmokeDoorObserved=true;UE_LOG(LogTemp,Display,TEXT("BR_CLIENT_SYNC role=%s case=DOOR_OPEN result=PASS"),*TestRole); }
    }
    if(Map.EndsWith(TEXT("L_MiddleFloor_Dark")))
    {
        auto Observe=[this](bool OK,FName Name)
        {if(OK && !ItemSmokeObserved.Contains(Name)){ItemSmokeObserved.Add(Name);UE_LOG(LogTemp,Display,TEXT("BR_ITEM_SYNC role=%s case=%s result=PASS"),*TestRole,*Name.ToString());}};
        if(FParse::Param(FCommandLine::Get(),TEXT("BRInventoryNet")))if(auto* Local=Cast<ABRPlayerCharacter>(GetPawn()))
        {
            auto* B=Local->GetInventoryComponent();Observe(B->GetSlots().Num()==12,TEXT("INVENTORY_TWELVE_SLOTS"));Observe(B->GetSanity()<99.9f,TEXT("INVENTORY_SAN_REPLICATED"));
            if(TestRole==TEXT("guest") && B->GetUsedSlots()==12 && !ItemSmokeObserved.Contains(TEXT("INVENTORY_USE_SENT")))
            {Observe(true,TEXT("INVENTORY_FULL_REPLICATED"));for(int32 I=0;I<12;++I)if(B->GetSlots()[I].Item==EBRInventoryItem::AlmondWater){auto Id=B->GetSlots()[I].InstanceId;B->ServerUseSlot(I,Id);B->ServerUseSlot(I,Id);Observe(true,TEXT("INVENTORY_USE_SENT"));break;}}
            if(TestRole==TEXT("guest") && ItemSmokeObserved.Contains(TEXT("INVENTORY_USE_SENT")))Observe(B->GetUsedSlots()==11 && B->GetSanity()>99,TEXT("INVENTORY_CONSUME_SAN_REPLICATED"));
            if(TestRole==TEXT("host"))for(TActorIterator<ABRPlayerCharacter> Other(GetWorld());Other;++Other)if(*Other!=Local && Other->GetInventoryComponent()->GetEquippedItem()!=EBRInventoryItem::Empty)
                Observe(Other->GetInventoryComponent()->GetUsedSlots()==0 && Other->GetInventoryComponent()->GetSanity()==100,TEXT("INVENTORY_PRIVATE_TO_OWNER"));
        }
        int32 Cabinets=0,KeyCabinets=0;
        for(TActorIterator<ABRLootCabinet> It(GetWorld());It;++It)
        {if(It->GetLootType()>=0)++Cabinets;if(It->GetLootType()==1)++KeyCabinets;}
        Observe(Cabinets==32 && KeyCabinets==4,TEXT("LOOT_32_KEYS_4"));
        for(TActorIterator<ABRPlayerCharacter> It(GetWorld());It;++It)
            if(auto* OwnerState=It->GetPlayerState<ABRPlayerState>())if(OwnerState->IsRoomOwner())
            {
                Observe(It->HasFlashlight() && It->IsFlashlightOn(),TEXT("OWNER_TORCH_ON"));
                Observe(ItemSmokeObserved.Contains(TEXT("OWNER_TORCH_ON")) && It->HasFlashlight() && !It->IsFlashlightOn(),TEXT("OWNER_TORCH_OFF"));
                Observe(ItemSmokeObserved.Contains(TEXT("OWNER_TORCH_OFF")) && It->IsFlashlightOn(),TEXT("OWNER_TORCH_ON_AGAIN"));
                if(auto* Local=Cast<ABRPlayerCharacter>(GetPawn()))Observe(It->HasFlashlight() && Local!=*It && !Local->HasFlashlight(),TEXT("SINGLE_OWNER"));
            }
        for(TActorIterator<ABRSupplyPickup> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("BR_EntryFlashlight")))
            Observe(It->IsCollected() && It->IsHidden() && !It->GetActorEnableCollision(),TEXT("ENTRY_PICKUP_REMOVED"));
        for(TActorIterator<ABRGarageDoor> It(GetWorld());It;++It)if(!It->IsA<ABRLootCabinet>() && It->IsOpen() && It->GetOpenAlpha()>0.99f)
        {ItemSmokeDoor=*It;Observe(true,TEXT("ROOM_DOOR_OPEN"));break;}
        Observe(ItemSmokeDoor.IsValid() && !ItemSmokeDoor->IsOpen() && ItemSmokeDoor->GetOpenAlpha()<0.01f,TEXT("ROOM_DOOR_CLOSED"));
    }
    double TestTimeout = 150.0;
    FParse::Value(FCommandLine::Get(), TEXT("BRTestTimeout="), TestTimeout);
    if (FPlatformTime::Seconds() - D->SmokeStarted > TestTimeout)
    {
        UE_LOG(LogTemp, Error, TEXT("BR_NETTEST role=%s result=TIMEOUT"), *TestRole);
        FPlatformMisc::RequestExitWithStatus(false, 2);
    }
#endif
}

void ABRMenuPlayerController::UpdateRoundResult()
{
    const auto* State=GetWorld()->GetGameState<ABRGameState>();
    if(RoundResult || !State || (State->GetLevelPhase()!=EBRLevelPhase::Completed && State->GetLevelPhase()!=EBRLevelPhase::Failed))return;
    if(!bRoundInputLocked)
    {
        CloseInventory();SetIgnoreMoveInput(true);SetIgnoreLookInput(true);bRoundInputLocked=true;
        if(auto* LocalPawn=Cast<ABRPlayerCharacter>(GetPawn())){LocalPawn->StopSprint();LocalPawn->GetCharacterMovement()->StopMovementImmediately();}
    }
    if(!FApp::CanEverRender())return;
    RoundResult=CreateWidget<UBRRoundResultWidget>(this,UBRRoundResultWidget::StaticClass());if(!RoundResult)return;
    if(GameHUD)GameHUD->SetVisibility(ESlateVisibility::Collapsed);
    RoundResult->AddToViewport(100);bShowMouseCursor=true;
    FInputModeUIOnly Mode;Mode.SetWidgetToFocus(RoundResult->TakeWidget());SetInputMode(Mode);
}

void ABRMenuPlayerController::EndPlay(const EEndPlayReason::Type Reason)
{
    // A retiring seamless-travel controller must not reset the new controller's input mode.
    if (Inventory) { Inventory->RemoveFromParent();Inventory=nullptr; }
    if (Menu) { Menu->RemoveFromParent();Menu=nullptr; }
    if (GameHUD) { GameHUD->RemoveFromParent();GameHUD=nullptr; }
    if (RoundResult) { RoundResult->RemoveFromParent();RoundResult=nullptr; }
    Super::EndPlay(Reason);
}

void ABRMenuPlayerController::ToggleInventory()
{
    if(!IsLocalController())return;if(Inventory){CloseInventory();return;}
    auto* P=Cast<ABRPlayerCharacter>(GetPawn());if(!P || P->GetDownedComponent()->IsDowned() || P->GetCurrentHideSpot() || !GameHUD || bRoundInputLocked)return;
    auto Class=LoadClass<UBRInventoryWidget>(nullptr,TEXT("/Game/UI/Inventory/Widgets/WBP_Inventory.WBP_Inventory_C"));if(!Class)return;
    Inventory=CreateWidget<UBRInventoryWidget>(this,Class);if(!Inventory)return;
    Inventory->AddToViewport(20);if(GameHUD)GameHUD->SetVisibility(ESlateVisibility::Collapsed);bShowMouseCursor=true;
    SetIgnoreMoveInput(true);SetIgnoreLookInput(true);P->GetCharacterMovement()->StopMovementImmediately();
    FInputModeGameAndUI Mode;Mode.SetWidgetToFocus(Inventory->TakeWidget());Mode.SetHideCursorDuringCapture(false);SetInputMode(Mode);Inventory->SetKeyboardFocus();
}
void ABRMenuPlayerController::CloseInventory()
{
    if(!Inventory)return;Inventory->RemoveFromParent();Inventory=nullptr;if(GameHUD)GameHUD->SetVisibility(ESlateVisibility::Visible);bShowMouseCursor=false;
    SetIgnoreMoveInput(false);SetIgnoreLookInput(false);SetInputMode(FInputModeGameOnly());
}
void ABRMenuPlayerController::QuickSlot(int32 I){if(auto* P=Cast<ABRPlayerCharacter>(GetPawn()))if(!Inventory)P->GetInventoryComponent()->SelectSlot(UBRInventoryComponent::PocketStart+I);}
void ABRMenuPlayerController::UseHeldItem()
{if(auto* P=Cast<ABRPlayerCharacter>(GetPawn()))if(!Inventory)
 {if(P->GetInteractionComponent()->FindFocusedInteractable())P->GetInteractionComponent()->TryInteract();
  else P->GetInventoryComponent()->UseSelected();}}
void ABRMenuPlayerController::DropHeldItem(){if(auto* P=Cast<ABRPlayerCharacter>(GetPawn()))if(!Inventory)P->GetInventoryComponent()->DropSelected();}
