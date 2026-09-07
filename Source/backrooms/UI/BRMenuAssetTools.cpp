#include "UI/BRMenuAssetTools.h"
#if WITH_EDITOR
#include "UI/BRMenuWidget.h"
#include "UI/BRGameHUDWidget.h"
#include "AssetToolsModule.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintFactory.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Components/WidgetSwitcher.h"
#include "Components/VerticalBox.h"
#include "Components/BackgroundBlur.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "Sound/SoundBase.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace BRMenuBuild
{
bool Save(UObject* Asset)
{
    FSavePackageArgs Args; Args.TopLevelFlags = RF_Public | RF_Standalone; Args.SaveFlags = SAVE_NoError;
    Asset->MarkPackageDirty();
    return UPackage::SavePackage(Asset->GetOutermost(), Asset, *FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(), FPackageName::GetAssetPackageExtension()), Args);
}
FSlateBrush Brush(const FString& Name)
{
    FSlateBrush B; B.DrawAs = ESlateBrushDrawType::Image;
    const FString Path = TEXT("/Game/UI/Menu/Art/") + Name + TEXT(".") + Name;
    if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *Path)) { B.SetResourceObject(T); B.ImageSize = FVector2D(T->GetSizeX(), T->GetSizeY()); }
    return B;
}
template<class T> T* Make(UWidgetTree* Tree, const TCHAR* Name)
{
    T* W = Tree->ConstructWidget<T>(T::StaticClass(), FName(Name)); W->bIsVariable = true;
    if (auto* BP = Tree->GetTypedOuter<UWidgetBlueprint>())
        if (!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName())) BP->OnVariableAdded(W->GetFName());
    return W;
}
void Place(UCanvasPanel* P, UWidget* W, float X, float Y, float Width, float Height)
{
    UCanvasPanelSlot* S = P->AddChildToCanvas(W); S->SetPosition(FVector2D(X,Y)); S->SetSize(FVector2D(Width,Height));
}
void Picture(UWidgetTree* T, UCanvasPanel* P, const TCHAR* Name, const TCHAR* Art, float X, float Y, float W, float H, float Opacity=1)
{
    UImage* I = Make<UImage>(T,Name); I->SetBrush(Brush(Art)); I->SetRenderOpacity(Opacity); I->SetVisibility(ESlateVisibility::HitTestInvisible); Place(P,I,X,Y,W,H);
}
UTextBlock* Text(UWidgetTree* T, UCanvasPanel* P, UFont* Font, const TCHAR* Name, const TCHAR* Label, float X, float Y, float W, float H, int32 Size, bool bDark=false)
{
    UTextBlock* L = Make<UTextBlock>(T,Name); L->SetText(FText::FromString(Label)); L->SetFont(FSlateFontInfo(Font,Size,TEXT("Regular")));
    L->SetColorAndOpacity(bDark ? FLinearColor(0.03f,0.035f,0.035f,1) : FLinearColor(0.92f,0.93f,0.92f,1));
    L->SetVisibility(ESlateVisibility::HitTestInvisible); Place(P,L,X,Y,W,H); return L;
}
UButton* Button(UWidgetTree* T, UCanvasPanel* P, UFont* Font, const TCHAR* Name, const TCHAR* Label, float X, float Y, float W, float H, bool bStrip=false, const TCHAR* TextName=nullptr)
{
    UButton* B = Make<UButton>(T,Name); FButtonStyle Style;
    Style.SetNormal(Brush(bStrip ? TEXT("T_TextEntry") : TEXT("T_Menu_Button")));
    Style.SetHovered(Brush(bStrip ? TEXT("T_TextEntry_Hovered") : TEXT("T_Menu_Button__Hovered")));
    Style.SetPressed(Brush(bStrip ? TEXT("T_TextEntry_Selected") : TEXT("T_Menu_Button__Pressed")));
    FSlateBrush Disabled=Brush(bStrip ? TEXT("T_TextEntry") : TEXT("T_Menu_Button"));
    Disabled.TintColor=FLinearColor(0.7f,0.7f,0.7f,1);Style.SetDisabled(Disabled);
    FSlateSound Click, Hover;
    Click.SetResourceObject(LoadObject<USoundBase>(nullptr,TEXT("/Game/UI/Menu/Audio/Wave_Click.Wave_Click")));
    Hover.SetResourceObject(LoadObject<USoundBase>(nullptr,TEXT("/Game/UI/Menu/Audio/Wave_Hovered.Wave_Hovered")));
    Style.SetPressedSound(Click); Style.SetHoveredSound(Hover); B->SetStyle(Style);
    UTextBlock* L = Make<UTextBlock>(T,TextName ? TextName : *(FString(Name)+TEXT("_Label")));
    L->SetText(FText::FromString(Label)); L->SetFont(FSlateFontInfo(Font,32,TEXT("Regular"))); L->SetColorAndOpacity(FLinearColor(0.025f,0.025f,0.025f,1));
    B->AddChild(L); Place(P,B,X,Y,W,H); return B;
}
UEditableTextBox* Entry(UWidgetTree* T, UCanvasPanel* P, UFont* Font, const TCHAR* Name, const TCHAR* Hint, float X, float Y, float W)
{
    auto* E = Make<UEditableTextBox>(T,Name); E->SetHintText(FText::FromString(Hint));
    FEditableTextBoxStyle S; S.SetFont(FSlateFontInfo(Font,28,TEXT("Regular"))); S.SetForegroundColor(FLinearColor::Black);
    S.SetBackgroundImageNormal(Brush(TEXT("T_TextEntry"))); S.SetBackgroundImageHovered(Brush(TEXT("T_TextEntry_Hovered")));
    S.SetBackgroundImageFocused(Brush(TEXT("T_TextEntry_Selected"))); S.SetPadding(FMargin(24,8)); E->SetWidgetStyle(S);
    Place(P,E,X,Y,W,64); return E;
}
}
#endif

bool UBRMenuAssetTools::BuildMenuAssets()
{
#if WITH_EDITOR
    using namespace BRMenuBuild;
    UFont* Font = LoadObject<UFont>(nullptr,TEXT("/Game/UI/Menu/Fonts/F_MenuChinese.F_MenuChinese"));
    if (!Font)
    {
        const FString Filename = FPaths::EngineContentDir()/TEXT("Slate/Fonts/DroidSansFallback.ttf");
        TArray<uint8> Bytes; if (!FFileHelper::LoadFileToArray(Bytes,*Filename)) return false;
        auto* Face = NewObject<UFontFace>(CreatePackage(TEXT("/Game/UI/Menu/Fonts/FF_MenuChinese")),TEXT("FF_MenuChinese"),RF_Public|RF_Standalone);
        Face->InitializeFromBulkData(Filename,EFontHinting::Default,Bytes.GetData(),Bytes.Num()); Face->LoadingPolicy = EFontLoadingPolicy::Inline;
        FAssetRegistryModule::AssetCreated(Face); if (!Save(Face)) return false;
        Font = NewObject<UFont>(CreatePackage(TEXT("/Game/UI/Menu/Fonts/F_MenuChinese")),TEXT("F_MenuChinese"),RF_Public|RF_Standalone);
        Font->FontCacheType = EFontCacheType::Runtime;
        FTypefaceEntry Entry(TEXT("Regular")); Entry.Font = FFontData(Face); Font->GetMutableInternalCompositeFont().DefaultTypeface.Fonts.Add(Entry);
        FAssetRegistryModule::AssetCreated(Font); if (!Save(Font)) return false;
    }
    auto* BP = LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Menu/Widgets/WBP_MainMenu.WBP_MainMenu"));
    if (!BP)
    {
        auto* Factory = NewObject<UWidgetBlueprintFactory>(); Factory->ParentClass = UBRMenuWidget::StaticClass();
        BP = Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(TEXT("WBP_MainMenu"),TEXT("/Game/UI/Menu/Widgets"),UWidgetBlueprint::StaticClass(),Factory));
    }
    if (!BP || BP->ParentClass != UBRMenuWidget::StaticClass()) return false;
    // Rebuilding this generated asset is explicit; its complete editable designer tree is saved.
    UWidgetTree* T = BP->WidgetTree;
    TArray<UWidget*> Previous; T->GetAllWidgets(Previous); T->RootWidget = nullptr;
    for (UWidget* W : Previous) W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    auto* Scale = Make<UScaleBox>(T,TEXT("ViewportScale")); Scale->SetStretch(EStretch::ScaleToFit); T->RootWidget = Scale;
    auto* Size = Make<USizeBox>(T,TEXT("DesignResolution")); Size->SetWidthOverride(1920); Size->SetHeightOverride(1080); Scale->AddChild(Size);
    auto* Root = Make<UCanvasPanel>(T,TEXT("MenuCanvas")); Size->AddChild(Root);
    Picture(T,Root,TEXT("BackroomsBackground"),TEXT("VHSFilter"),0,0,1920,1080);
    auto* Blur = Make<UBackgroundBlur>(T,TEXT("PageBlur")); Blur->SetBlurStrength(5.0f);
    Blur->SetVisibility(ESlateVisibility::Collapsed); Place(Root,Blur,0,0,1920,1080);
    Picture(T,Root,TEXT("CrackedGlassOverlay"),TEXT("T_Menu_BG_01"),0,0,1920,1080,0.65f);
    auto* Pages = Make<UWidgetSwitcher>(T,TEXT("Pages")); Place(Root,Pages,0,0,1920,1080);
    auto* Main = Make<UCanvasPanel>(T,TEXT("MainPage")); Pages->AddChild(Main);
    auto* Create = Make<UCanvasPanel>(T,TEXT("CreatePage")); Pages->AddChild(Create);
    auto* Lobby = Make<UCanvasPanel>(T,TEXT("LobbyPage")); Pages->AddChild(Lobby);
    auto* Browse = Make<UCanvasPanel>(T,TEXT("BrowsePage")); Pages->AddChild(Browse);
    Text(T,Main,Font,TEXT("GameTitle"),TEXT("BACKROOMS"),110,105,1100,150,92);
    Text(T,Main,Font,TEXT("GameSubtitle"),TEXT("停车场 · 多人逃生 DEMO"),120,270,1000,65,32);
    Button(T,Main,Font,TEXT("PlayButton"),TEXT("玩游戏"),120,430,490,72,true);
    Button(T,Main,Font,TEXT("BrowseButton"),TEXT("加入游戏"),120,520,490,72,true);
    Button(T,Main,Font,TEXT("QuitButton"),TEXT("退出"),120,720,490,72,true);
    Text(T,Main,Font,TEXT("MainNetworkHint"),TEXT("独立服务器 / 1–4 人合作"),1250,100,570,50,26);
    Text(T,Create,Font,TEXT("CreateHeading"),TEXT("玩游戏"),110,75,1000,90,52);
    Picture(T,Create,TEXT("StoryPanel"),TEXT("T_bgpanel_mid"),340,205,625,640);
    Picture(T,Create,TEXT("StoryImage"),TEXT("VHSFilter"),365,240,575,400);
    Text(T,Create,Font,TEXT("StoryDescription"),TEXT("寻找钥匙 · 躲避窃皮者 · 电梯逃生"),380,665,550,65,25);
    Button(T,Create,Font,TEXT("StoryButton"),TEXT("故事模式 · 创建房间"),375,750,550,72,true);
    Text(T,Create,Font,TEXT("RoomNameLabel"),TEXT("房间名称"),1100,300,560,55,30);
    Entry(T,Create,Font,TEXT("RoomNameBox"),TEXT("输入房间名称…"),1090,375,620);
    Text(T,Create,Font,TEXT("CapacityLabel"),TEXT("最大玩家数"),1100,490,500,55,30);
    auto* Capacity = Make<UComboBoxString>(T,TEXT("CapacityBox")); for (int32 I=1;I<=4;++I) Capacity->AddOption(FString::FromInt(I));
    Capacity->SetSelectedOption(TEXT("4")); Place(Create,Capacity,1100,565,220,64);
    Text(T,Create,Font,TEXT("SingleRoomHint"),TEXT("当前服务器同时支持一个房间。\n游戏开始后等待下一局加入。"),1100,680,650,140,25);
    Button(T,Create,Font,TEXT("CreateBackButton"),TEXT("返回"),110,900,300,70);
    Picture(T,Lobby,TEXT("LobbyTitleStrip"),TEXT("T_TextEntry"),30,65,650,90);
    Text(T,Lobby,Font,TEXT("LobbyHeading"),TEXT("大厅"),90,78,450,65,48,true);
    Picture(T,Lobby,TEXT("PlayersPanel"),TEXT("T_bgpanel_mid_scrollbox"),145,245,850,560);
    auto* Players = Make<UVerticalBox>(T,TEXT("LobbyPlayers")); Place(Lobby,Players,185,280,770,460);
    Picture(T,Lobby,TEXT("LobbyPreview"),TEXT("T_ParkingPreview"),1120,220,650,355);
    Text(T,Lobby,Font,TEXT("LobbySummary"),TEXT("正在同步大厅…"),1120,610,660,160,29);
    Button(T,Lobby,Font,TEXT("ReadyButton"),TEXT("准备"),1250,810,380,75,false,TEXT("ReadyLabel"));
    Button(T,Lobby,Font,TEXT("StartButton"),TEXT("开始游戏"),1250,810,380,75);
    Button(T,Lobby,Font,TEXT("LeaveButton"),TEXT("离开房间"),110,900,350,70);
    Picture(T,Browse,TEXT("BrowseTitleStrip"),TEXT("T_TextEntry"),25,70,700,90);
    Text(T,Browse,Font,TEXT("BrowseHeading"),TEXT("服务器列表"),70,82,620,65,46,true);
    Text(T,Browse,Font,TEXT("BrowseSummary"),TEXT("正在查询…"),650,180,1110,55,29);
    Button(T,Browse,Font,TEXT("RefreshButton"),TEXT("刷新列表"),1490,85,340,65);
    Picture(T,Browse,TEXT("ServerListPanel"),TEXT("T_bgpanel_mid_scrollbox"),595,250,1250,550);
    auto* Row = Make<UCanvasPanel>(T,TEXT("RoomRow")); Place(Browse,Row,640,300,1160,130);
    Picture(T,Row,TEXT("RoomRowBackground"),TEXT("T_BarreH"),0,0,1160,125,0.45f);
    Text(T,Row,Font,TEXT("RoomTitle"),TEXT("等待房间信息"),20,16,900,105,24)->SetAutoWrapText(true);
    Button(T,Row,Font,TEXT("JoinButton"),TEXT("加入"),940,30,190,65);
    Row->SetVisibility(ESlateVisibility::Collapsed);
    Text(T,Browse,Font,TEXT("BrowseHelp"),TEXT("房间由独立服务器运行。\n\n房主开始后，所有玩家\n一起进入停车场。"),110,375,460,300,28);
    Button(T,Browse,Font,TEXT("BrowseBackButton"),TEXT("返回"),110,900,300,70);
    Text(T,Root,Font,TEXT("NicknameLabel"),TEXT("昵称"),1195,941,130,55,28);
    Entry(T,Root,Font,TEXT("NicknameBox"),TEXT("首次联机请填写昵称"),1320,930,470);
    Text(T,Root,Font,TEXT("StatusText"),TEXT(""),110,1020,1700,48,23);
    Pages->SetActiveWidgetIndex(0);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP); FKismetEditorUtilities::CompileBlueprint(BP);
    if (BP->Status == BS_Error) return false;
    auto* Defaults = Cast<UBRMenuWidget>(BP->GeneratedClass->GetDefaultObject()); Defaults->ChineseFont = Font;
    if (!Save(BP)) return false;
    auto* HUD=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Menu/Widgets/WBP_GameHUD.WBP_GameHUD"));
    if (!HUD)
    {
        auto* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=UBRGameHUDWidget::StaticClass();
        HUD=Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(TEXT("WBP_GameHUD"),TEXT("/Game/UI/Menu/Widgets"),UWidgetBlueprint::StaticClass(),Factory));
    }
    if (!HUD) return false;
    UWidgetTree* H=HUD->WidgetTree;TArray<UWidget*> OldHUD;H->GetAllWidgets(OldHUD);H->RootWidget=nullptr;
    for (UWidget* W:OldHUD) W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    auto* HS=Make<UScaleBox>(H,TEXT("HUDScale"));HS->SetStretch(EStretch::ScaleToFit);H->RootWidget=HS;
    auto* HB=Make<USizeBox>(H,TEXT("HUDDesignSize"));HB->SetWidthOverride(1920);HB->SetHeightOverride(1080);HS->AddChild(HB);
    auto* HC=Make<UCanvasPanel>(H,TEXT("HUDCanvas"));HB->AddChild(HC);
    Text(H,HC,Font,TEXT("KeyStatus"),TEXT(""),45,40,1250,70,28);
    auto* Hint=Text(H,HC,Font,TEXT("InteractionHint"),TEXT(""),450,865,1020,80,30);Hint->SetJustification(ETextJustify::Center);
    auto* Round=Text(H,HC,Font,TEXT("RoundStatus"),TEXT(""),400,420,1120,220,48);Round->SetJustification(ETextJustify::Center);
    Text(H,HC,Font,TEXT("LeaveHint"),TEXT("Esc · 离开房间"),45,1000,450,55,22);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(HUD);FKismetEditorUtilities::CompileBlueprint(HUD);
    if (HUD->Status==BS_Error || !Save(HUD)) return false;
    UE_LOG(LogTemp, Display, TEXT("BR_UI_ASSETS result=PASS pages=4 editable_widget_blueprint=true hud=true")); return true;
#else
    return false;
#endif
}
