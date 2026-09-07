#include "UI/BRInventoryAssetTools.h"
#if WITH_EDITOR
#include "UI/BRInventoryWidget.h"
#include "AssetToolsModule.h"
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
#include "Components/ProgressBar.h"
#include "Components/BackgroundBlur.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
namespace BRInventoryBuild
{
FSlateBrush Brush(const FString& Name)
{
    FSlateBrush B;B.DrawAs=ESlateBrushDrawType::Image;
    FString Path=TEXT("/Game/UI/Inventory/Art/")+Name+TEXT(".")+Name;
    if(UTexture2D* T=LoadObject<UTexture2D>(nullptr,*Path)){B.SetResourceObject(T);B.ImageSize=FVector2D(T->GetSizeX(),T->GetSizeY());}return B;
}
template<class T>T* Make(UWidgetTree* Tree,const FString& Name)
{
    auto* W=Tree->ConstructWidget<T>(T::StaticClass(),FName(*Name));W->bIsVariable=true;
    if(auto* BP=Tree->GetTypedOuter<UWidgetBlueprint>())if(!BP->WidgetVariableNameToGuidMap.Contains(W->GetFName()))BP->OnVariableAdded(W->GetFName());return W;
}
void Place(UCanvasPanel* C,UWidget* W,float X,float Y,float Width,float Height)
{auto* S=C->AddChildToCanvas(W);S->SetPosition(FVector2D(X,Y));S->SetSize(FVector2D(Width,Height));}
UTextBlock* Label(UWidgetTree* T,UCanvasPanel* C,UFont* F,const FString& N,const FString& V,float X,float Y,float W,float H,int32 FontSize=26)
{
    auto* L=Make<UTextBlock>(T,N);L->SetText(FText::FromString(V));L->SetFont(FSlateFontInfo(F,FontSize,TEXT("Regular")));L->SetVisibility(ESlateVisibility::HitTestInvisible);L->SetColorAndOpacity(FLinearColor(0.9f,0.92f,0.89f,1));Place(C,L,X,Y,W,H);return L;
}
UImage* Picture(UWidgetTree* T,UCanvasPanel* C,const FString& N,const FString& A,float X,float Y,float W,float H)
{auto* I=Make<UImage>(T,N);I->SetBrush(Brush(A));I->SetVisibility(ESlateVisibility::HitTestInvisible);Place(C,I,X,Y,W,H);return I;}
UButton* Button(UWidgetTree* T,UCanvasPanel* C,UFont* F,const FString& N,const FString& V,float X,float Y,float W,float H)
{
    auto* B=Make<UButton>(T,N);FButtonStyle S;S.SetNormal(Brush(TEXT("Full_streatch_background_9_patch")));S.SetHovered(S.Normal);S.Hovered.TintColor=FLinearColor(0.5f,0.7f,0.7f,1);S.SetPressed(S.Hovered);S.Pressed.TintColor=FLinearColor(0.8f,0.9f,0.9f,1);B->SetStyle(S);if(auto* FP=FindFProperty<FBoolProperty>(UButton::StaticClass(),TEXT("IsFocusable")))FP->SetPropertyValue_InContainer(B,false);
    auto* Text=Make<UTextBlock>(T,N+TEXT("Text"));Text->SetText(FText::FromString(V));Text->SetFont(FSlateFontInfo(F,26,TEXT("Regular")));B->AddChild(Text);Place(C,B,X,Y,W,H);return B;
}
bool Save(UObject* Asset)
{Asset->MarkPackageDirty();FSavePackageArgs A;A.TopLevelFlags=RF_Public|RF_Standalone;return UPackage::SavePackage(Asset->GetOutermost(),Asset,*FPackageName::LongPackageNameToFilename(Asset->GetOutermost()->GetName(),FPackageName::GetAssetPackageExtension()),A);}
}
#endif
bool UBRInventoryAssetTools::BuildInventoryAsset()
{
#if WITH_EDITOR
    using namespace BRInventoryBuild;
    auto* F=LoadObject<UFont>(nullptr,TEXT("/Game/UI/Menu/Fonts/F_MenuChinese.F_MenuChinese"));if(!F)return false;
    auto* BP=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Inventory/Widgets/WBP_Inventory.WBP_Inventory"));
    if(!BP){auto* Factory=NewObject<UWidgetBlueprintFactory>();Factory->ParentClass=UBRInventoryWidget::StaticClass();BP=Cast<UWidgetBlueprint>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(TEXT("WBP_Inventory"),TEXT("/Game/UI/Inventory/Widgets"),UWidgetBlueprint::StaticClass(),Factory));}
    if(!BP || BP->ParentClass!=UBRInventoryWidget::StaticClass())return false;
    UWidgetTree* T=BP->WidgetTree;TArray<UWidget*> Old;T->GetAllWidgets(Old);T->RootWidget=nullptr;BP->WidgetVariableNameToGuidMap.Reset();for(auto* W:Old)W->Rename(nullptr,GetTransientPackage(),REN_DontCreateRedirectors|REN_NonTransactional);
    auto* Scale=Make<UScaleBox>(T,TEXT("InventoryScale"));Scale->SetStretch(EStretch::ScaleToFit);T->RootWidget=Scale;
    auto* Size=Make<USizeBox>(T,TEXT("InventoryResolution"));Size->SetWidthOverride(1920);Size->SetHeightOverride(1080);Scale->AddChild(Size);
    auto* C=Make<UCanvasPanel>(T,TEXT("InventoryCanvas"));Size->AddChild(C);
    C->SetVisibility(ESlateVisibility::Visible);
    auto* Blur=Make<UBackgroundBlur>(T,TEXT("InventoryBlur"));Blur->SetBlurStrength(1);Blur->SetVisibility(ESlateVisibility::HitTestInvisible);Place(C,Blur,0,0,1920,1080);
    auto* Dim=Make<UImage>(T,TEXT("DimBackdrop"));Dim->SetBrush(FSlateColorBrush(FLinearColor(0.18f,0.19f,0.22f,0.64f)));Dim->SetVisibility(ESlateVisibility::HitTestInvisible);Place(C,Dim,0,0,1920,1080);
    Picture(T,C,TEXT("CrackedOverlay"),TEXT("BackgroundOverlay"),0,0,1920,1080)->SetOpacity(0.65f);
    auto Title=[&](const TCHAR* N,const TCHAR* V,float X,float Y,float W)
    {Picture(T,C,FString(N)+TEXT("Strip"),TEXT("PanelTitle"),X,Y,W,46);auto* L=Label(T,C,F,N,V,X+10,Y+4,W-25,42,30);L->SetJustification(ETextJustify::Right);L->SetColorAndOpacity(FLinearColor::Black);};
    Picture(T,C,TEXT("MainTitleStrip"),TEXT("MainTitleBG"),1298,0,622,100);
    auto* MT=Label(T,C,F,TEXT("InventoryTitle"),TEXT("Inventory"),1490,30,360,60,32);MT->SetJustification(ETextJustify::Right);MT->SetColorAndOpacity(FLinearColor::Black);
    Picture(T,C,TEXT("QuickPanel"),TEXT("BagpackBG"),205,150,390,825)->SetOpacity(0.38f);
    Title(TEXT("QuickTitle"),TEXT("快捷栏"),220,140,370);
    Title(TEXT("HandTitle"),TEXT("主手"),270,270,270);
    Title(TEXT("PocketsTitle"),TEXT("口袋"),270,540,270);
    Title(TEXT("CapacityText"),TEXT("背包"),1030,260,270);
    Picture(T,C,TEXT("BackpackPanel"),TEXT("BagpackBG"),797,301,516,516)->SetOpacity(0.55f);
    for(int32 I=0;I<12;++I)
    {
        const float X=I<9?805+(I%3)*168:325;
        const float Y=I<9?310+(I/3)*168:I==9?335:605+(I-10)*168;
        auto* B=Button(T,C,F,FString::Printf(TEXT("Slot%d"),I),TEXT(""),X,Y,160,160);
        FButtonStyle Style=B->GetStyle();Style.SetNormal(Brush(TEXT("DeactiveIconBG")));Style.SetHovered(Style.Normal);Style.SetPressed(Style.Normal);B->SetStyle(Style);
        // Parent handles mouse down/drag/up so clicks and drags are mutually exclusive.
        B->SetVisibility(ESlateVisibility::HitTestInvisible);
        Picture(T,C,FString::Printf(TEXT("ItemIcon%d"),I),TEXT("Flashlight"),X+5,Y+5,150,150)->SetVisibility(ESlateVisibility::Hidden);
        auto* BI=Make<UCanvasPanel>(T,FString::Printf(TEXT("BatteryIcon%d"),I));Place(C,BI,X+40,Y+50,92,60);BI->SetVisibility(ESlateVisibility::HitTestInvisible);
        auto Solid=[T](UCanvasPanel* Parent,const FString& Name,float SX,float SY,float SW,float SH,FLinearColor Color){auto* Img=Make<UImage>(T,Name);Img->SetBrush(FSlateColorBrush(Color));Img->SetVisibility(ESlateVisibility::HitTestInvisible);Place(Parent,Img,SX,SY,SW,SH);};
        const FString BN=FString::Printf(TEXT("Battery%d"),I);
        Solid(BI,BN+TEXT("Body"),0,0,75,48,FLinearColor(.6f,.7f,.65f,1));Solid(BI,BN+TEXT("Inside"),5,5,65,38,FLinearColor(.05f,.06f,.06f,1));Solid(BI,BN+TEXT("Charge"),10,10,48,28,FLinearColor(.6f,.7f,.65f,1));Solid(BI,BN+TEXT("Terminal"),75,14,8,20,FLinearColor(.6f,.7f,.65f,1));
        Solid(C,FString::Printf(TEXT("SelectedMark%d"),I),X,Y+158,160,2,FLinearColor(.8f,.9f,.9f,1));
    }
    Picture(T,C,TEXT("SanityFrame"),TEXT("cropassets__1_"),1440,265,291,598);
    Picture(T,C,TEXT("SanityBacking"),TEXT("Option_1"),1465,305,242,515);
    Picture(T,C,TEXT("SanityBrain"),TEXT("Level_1"),1479,370,214,341);
    auto* Bar=Make<UProgressBar>(T,TEXT("SanityBar"));Bar->SetBarFillType(EProgressBarFillType::BottomToTop);Bar->SetPercent(1);
    FProgressBarStyle BS;BS.BackgroundImage=FSlateColorBrush(FLinearColor(.15f,.2f,.22f,.7f));BS.FillImage=FSlateColorBrush(FLinearColor::White);Bar->SetWidgetStyle(BS);Bar->SetFillColorAndOpacity(FLinearColor(.35f,.45f,.47f,.75f));Bar->SetVisibility(ESlateVisibility::HitTestInvisible);Place(C,Bar,1475,700,221,112);
    Label(T,C,F,TEXT("SanityText"),TEXT("SAN 100 / 100"),1460,850,290,42,23)->SetJustification(ETextJustify::Center);
    Picture(T,C,TEXT("FooterStrip"),TEXT("keyInstructor"),980,1015,940,47);
    auto* Help=Label(T,C,F,TEXT("InventoryInstructions"),TEXT("拖动 / 交换 · 单击装备 · 右键使用 · G 丢弃 · Tab 收起"),1000,1020,900,40,22);Help->SetColorAndOpacity(FLinearColor::Black);Help->SetJustification(ETextJustify::Right);
    Button(T,C,F,TEXT("CloseButton"),TEXT("返回"),220,990,220,48);
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(BP);FKismetEditorUtilities::CompileBlueprint(BP);if(BP->Status==BS_Error)return false;
    auto* Defaults=CastChecked<UBRInventoryWidget>(BP->GeneratedClass->GetDefaultObject());
    Defaults->FlashlightIcon=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Inventory/Art/Flashlight.Flashlight"));Defaults->AlmondWaterIcon=LoadObject<UTexture2D>(nullptr,TEXT("/Game/UI/Inventory/Art/T_UI_Inv_Icon_AlmondWater.T_UI_Inv_Icon_AlmondWater"));
    if(!Save(BP))return false;
    // Add only these HUD fields; do not rebuild the user's menu pages.
    auto* HUD=LoadObject<UWidgetBlueprint>(nullptr,TEXT("/Game/UI/Menu/Widgets/WBP_GameHUD.WBP_GameHUD"));if(!HUD)return false;
    auto* HC=Cast<UCanvasPanel>(HUD->WidgetTree->FindWidget(TEXT("HUDCanvas")));if(!HC)return false;
    if(!HUD->WidgetTree->FindWidget(TEXT("SanityStatus")))Label(HUD->WidgetTree,HC,F,TEXT("SanityStatus"),TEXT("SAN 100 / 100\n[Tab] 背包 0 / 8"),1490,45,370,105,25);
    if(auto* K=HUD->WidgetTree->FindWidget(TEXT("KeyStatus")))if(auto* Slot=Cast<UCanvasPanelSlot>(K->Slot))Slot->SetSize(FVector2D(1300,120));
    FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(HUD);FKismetEditorUtilities::CompileBlueprint(HUD);if(HUD->Status==BS_Error || !Save(HUD))return false;
    UE_LOG(LogTemp,Display,TEXT("BR_INVENTORY_ASSET result=PASS editable=1 backpack=9 main_hand=1 pockets=2 sanity=1"));return true;
#else
    return false;
#endif
}
