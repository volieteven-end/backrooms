#include "UI/BRMenuWidget.h"
#include "UI/BRMenuPlayerController.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Online/BRRoomSettings.h"
#include "Core/BRGameState.h"
#include "Player/BRPlayerState.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/BackgroundBlur.h"
#include "Engine/GameInstance.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Misc/ConfigCacheIni.h"
#include "Sound/SoundBase.h"

void UBRMenuWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    PlayButton->OnClicked.AddDynamic(this, &ThisClass::OpenPlay);
    BrowseButton->OnClicked.AddDynamic(this, &ThisClass::OpenBrowse);
    QuitButton->OnClicked.AddDynamic(this, &ThisClass::Quit);
    StoryButton->OnClicked.AddDynamic(this, &ThisClass::Create);
    CreateBackButton->OnClicked.AddDynamic(this, &ThisClass::Back);
    BrowseBackButton->OnClicked.AddDynamic(this, &ThisClass::Back);
    RefreshButton->OnClicked.AddDynamic(this, &ThisClass::Refresh);
    JoinButton->OnClicked.AddDynamic(this, &ThisClass::Join);
    ReadyButton->OnClicked.AddDynamic(this, &ThisClass::Ready);
    StartButton->OnClicked.AddDynamic(this, &ThisClass::Start);
    LeaveButton->OnClicked.AddDynamic(this, &ThisClass::Leave);
    auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>();
    D->OnDirectoryChanged.AddDynamic(this, &ThisClass::DirectoryChanged);
    FString Nickname; GConfig->GetString(TEXT("Backrooms.LocalPlayer"), TEXT("Nickname"), Nickname, GGameUserSettingsIni);
    NicknameBox->SetText(FText::FromString(Nickname));
    // AddOption affects runtime options, not the serialized DefaultOptions array.
    CapacityBox->OnGenerateWidgetEvent.BindDynamic(this,&ThisClass::GenerateCapacityItem);
    CapacityBox->ClearOptions();
    for (int32 I=1;I<=4;++I) CapacityBox->AddOption(FString::FromInt(I));
    CapacityBox->SetSelectedOption(TEXT("4"));
    ShowStatus(D->LastReply.Message);
}
void UBRMenuWidget::NativeDestruct()
{
    if (GetGameInstance()) GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->OnDirectoryChanged.RemoveDynamic(this, &ThisClass::DirectoryChanged);
    Super::NativeDestruct();
}
void UBRMenuWidget::SetPage(int32 Page)
{
    Pages->SetActiveWidgetIndex(FMath::Clamp(Page, 0, 3));
    PageBlur->SetVisibility(Page == 0 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    NicknameBox->SetIsReadOnly(Page == 2);
    NicknameBox->SetVisibility(Page==2 ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    if (auto* Label=WidgetTree->FindWidget(TEXT("NicknameLabel"))) Label->SetVisibility(Page==2 ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    if (Page == 3) Refresh();
    if (Page == 2) UpdateLobby();
}
void UBRMenuWidget::ShowStatus(const FString& Message) { StatusText->SetText(FText::FromString(Message)); }
UWidget* UBRMenuWidget::GenerateCapacityItem(FString Item)
{
    auto* Label=WidgetTree->ConstructWidget<UTextBlock>(); Label->SetText(FText::FromString(Item));
    Label->SetFont(FSlateFontInfo(ChineseFont,28,TEXT("Regular")));Label->SetColorAndOpacity(FLinearColor::Black);return Label;
}
void UBRMenuWidget::ClickSound()
{
    if (auto* Sound = LoadObject<USoundBase>(nullptr, TEXT("/Game/UI/Menu/Audio/Wave_Click.Wave_Click"))) UGameplayStatics::PlaySound2D(this, Sound);
}
FString UBRMenuWidget::ReadNickname()
{
    FString Nickname = NicknameBox->GetText().ToString().TrimStartAndEnd();
    if (!FBRRoomModel::ValidName(Nickname, 24)) { ShowStatus(TEXT("请先填写右下方昵称（1–24 个字符）。")); return FString(); }
    GConfig->SetString(TEXT("Backrooms.LocalPlayer"), TEXT("Nickname"), *Nickname, GGameUserSettingsIni);
    GConfig->Flush(false, GGameUserSettingsIni); return Nickname;
}
void UBRMenuWidget::OpenPlay() { SetPage(1); }
void UBRMenuWidget::OpenBrowse() { SetPage(3); }
void UBRMenuWidget::Back() { auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>(); if (D->bBusy) D->LeaveRoom(); else SetPage(0); }
void UBRMenuWidget::Quit() { UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, false); }
void UBRMenuWidget::Create()
{
    FString Nickname = ReadNickname(); if (Nickname.IsEmpty()) return;
    FString Name = RoomNameBox->GetText().ToString().TrimStartAndEnd();
    if (Name.IsEmpty()) Name = Nickname + TEXT(" 的房间");
    ShowStatus(TEXT("正在创建房间…"));
    GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->CreateRoom(Name, Nickname, FCString::Atoi(*CapacityBox->GetSelectedOption()));
}
void UBRMenuWidget::Refresh()
{
    NextRefresh = FPlatformTime::Seconds() + 5.0;
    auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>();
    if (!D->bBusy) { JoinButton->SetIsEnabled(false); ShowStatus(TEXT("正在查询服务器…")); D->QueryRooms(); }
}
void UBRMenuWidget::Join()
{
    FString Nickname = ReadNickname(); if (Nickname.IsEmpty()) return;
    GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->JoinRoom(DisplayedRoomId, Nickname);
}
void UBRMenuWidget::Ready()
{
    if (auto* PC = Cast<ABRMenuPlayerController>(GetOwningPlayer())) if (auto* PS = PC->GetPlayerState<ABRPlayerState>()) PC->ServerSetLobbyReady(!PS->IsReady());
}
void UBRMenuWidget::Start() { if (auto* PC = Cast<ABRMenuPlayerController>(GetOwningPlayer())) PC->ServerStartRoom(); }
void UBRMenuWidget::Leave() { GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->LeaveRoom(); }
void UBRMenuWidget::DirectoryChanged(const FBRDirectoryReply& Result)
{
    ShowStatus(Result.Message);
    if (!Result.bSuccess) { JoinButton->SetIsEnabled(false); return; }
    const bool bRoom = Result.Room.RoomId.IsValid();
    DisplayedRoomId = Result.Room.RoomId; RoomRow->SetVisibility(bRoom ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    BrowseSummary->SetText(FText::FromString(bRoom ? TEXT("已找到房间：1") : TEXT("暂无房间，可以返回创建游戏。")));
    const FString Phase = Result.Room.Phase == EBRRoomPhase::Lobby ? TEXT("等待玩家") : TEXT("游戏进行中");
    RoomTitle->SetText(FText::FromString(FString::Printf(TEXT("%s\n%d/%d 人    %s    连接延迟 %d ms"), *Result.Room.Name, Result.Room.CurrentPlayers, Result.Room.MaxPlayers, *Phase, Result.Room.PingMs)));
    JoinButton->SetIsEnabled(bRoom && Result.Room.bJoinable);
}
void UBRMenuWidget::UpdateLobby()
{
    auto* GS = GetWorld()->GetGameState<ABRGameState>(); auto* PS = GetOwningPlayerState<ABRPlayerState>();
    if (!GS || !PS) return;
    FString Roster; bool bAllReady = true;
    for (APlayerState* Base : GS->PlayerArray) if (auto* Member = Cast<ABRPlayerState>(Base))
    {
        if (!Member->GetRoomPlayerId().IsValid()) continue;
        Roster += FString::Printf(TEXT("%s  %s       %s\n"), Member->IsRoomOwner() ? TEXT("[房主]") : TEXT("[队员]"), *Member->GetPlayerName(), Member->IsReady() ? TEXT("准备就绪") : TEXT("未准备"));
        bAllReady &= Member->IsReady();
    }
    if (Roster != LastRoster)
    {
        LastRoster = Roster; LobbyPlayers->ClearChildren();
        for (APlayerState* Base : GS->PlayerArray)
        {
            auto* Member = Cast<ABRPlayerState>(Base);
            if (!Member || !Member->GetRoomPlayerId().IsValid()) continue;
            auto* Size = WidgetTree->ConstructWidget<USizeBox>(); Size->SetHeightOverride(100);
            auto* Row = WidgetTree->ConstructWidget<UCanvasPanel>(); Size->AddChild(Row);
            auto* Name = WidgetTree->ConstructWidget<UTextBlock>();
            auto* ReadyState = WidgetTree->ConstructWidget<UTextBlock>();
            Name->SetText(FText::FromString((Member->IsRoomOwner() ? TEXT("[房主] ") : TEXT("[队员] ")) + Member->GetPlayerName()));
            ReadyState->SetText(FText::FromString(Member->IsReady() ? TEXT("准备就绪") : TEXT("未准备")));
            FSlateFontInfo Font(ChineseFont,24,TEXT("Regular")); Name->SetFont(Font); ReadyState->SetFont(Font);
            Name->SetAutoWrapText(true);
            auto* NS = Row->AddChildToCanvas(Name); NS->SetPosition(FVector2D(10,10)); NS->SetSize(FVector2D(490,80));
            auto* RS = Row->AddChildToCanvas(ReadyState); RS->SetPosition(FVector2D(550,22)); RS->SetSize(FVector2D(190,50));
            LobbyPlayers->AddChildToVerticalBox(Size);
        }
    }
    LobbySummary->SetText(FText::FromString(FString::Printf(TEXT("%s\n%d / %d 人 · 故事模式 · 停车场\n%s"), *GS->RoomInfo.Name, GS->RoomInfo.CurrentPlayers, GS->RoomInfo.MaxPlayers,
        GS->RoomInfo.Phase == EBRRoomPhase::Lobby ? TEXT("等待队员准备") : TEXT("正在加载关卡…"))));
    const bool bLobby = GS->RoomInfo.Phase == EBRRoomPhase::Lobby;
    ReadyButton->SetVisibility(PS->IsRoomOwner() ? ESlateVisibility::Collapsed : ESlateVisibility::Visible);
    ReadyButton->SetIsEnabled(bLobby); ReadyLabel->SetText(FText::FromString(PS->IsReady() ? TEXT("取消准备") : TEXT("准备")));
    StartButton->SetVisibility(PS->IsRoomOwner() ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
    StartButton->SetIsEnabled(bLobby && bAllReady && GS->RoomInfo.CurrentPlayers > 0);
}
void UBRMenuWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry, DeltaTime);
    auto* D = GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>();
    StoryButton->SetIsEnabled(!D->bBusy); RefreshButton->SetIsEnabled(!D->bBusy);
    if (D->bBusy) JoinButton->SetIsEnabled(false);
    const double Now = FPlatformTime::Seconds();
    if (Pages->GetActiveWidgetIndex() == 3 && Now >= NextRefresh) Refresh();
    if (Pages->GetActiveWidgetIndex() == 2 && Now >= NextLobbyUpdate) { NextLobbyUpdate = Now + 0.25; UpdateLobby(); }
}
