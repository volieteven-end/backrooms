#include "UI/BRRoundResultWidget.h"
#include "UI/BRMenuPlayerController.h"
#include "Core/BRGameState.h"
#include "World/BRGarageKeyManager.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Font.h"
#include "EngineUtils.h"

void UBRRoundResultWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();SetIsFocusable(true);
    // Build the native tree before RebuildWidget chooses its Slate root.
    if(WidgetTree->RootWidget)return;
    auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(),TEXT("ResultRoot"));WidgetTree->RootWidget=Root;
    auto* Dim=WidgetTree->ConstructWidget<UImage>();Dim->SetBrush(FSlateColorBrush(FLinearColor(0.006f,0.011f,0.009f,0.96f)));
    auto* DimSlot=Root->AddChildToCanvas(Dim);DimSlot->SetAnchors(FAnchors(0,0,1,1));DimSlot->SetOffsets(FMargin(0));
    auto* Scale=WidgetTree->ConstructWidget<UScaleBox>();Scale->SetStretch(EStretch::ScaleToFit);
    auto* ScaleSlot=Root->AddChildToCanvas(Scale);ScaleSlot->SetAnchors(FAnchors(0,0,1,1));ScaleSlot->SetOffsets(FMargin(0));
    auto* Size=WidgetTree->ConstructWidget<USizeBox>();Size->SetWidthOverride(1920);Size->SetHeightOverride(1080);Scale->AddChild(Size);
    auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();Size->AddChild(Canvas);
    auto* Font=LoadObject<UFont>(nullptr,TEXT("/Game/UI/Menu/Fonts/F_MenuChinese.F_MenuChinese"));
    const auto* State=GetWorld()->GetGameState<ABRGameState>();
    const bool Won=State && State->GetLevelPhase()==EBRLevelPhase::Completed;
    const FLinearColor Accent=Won?FLinearColor(0.76f,0.82f,0.43f):FLinearColor(0.85f,0.36f,0.28f);
    auto Place=[Canvas](UWidget* Widget,float X,float Y,float Width,float Height)
    {auto* Cell=Canvas->AddChildToCanvas(Widget);Cell->SetPosition(FVector2D(X,Y));Cell->SetSize(FVector2D(Width,Height));};
    auto Label=[&](const TCHAR* Name,const FString& Value,float Y,int32 Points,FLinearColor Color)
    {
        auto* Text=WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),Name);Text->SetText(FText::FromString(Value));
        Text->SetFont(FSlateFontInfo(Font,Points,TEXT("Regular")));Text->SetJustification(ETextJustify::Center);Text->SetColorAndOpacity(Color);
        Place(Text,360,Y,1200,Points*1.7f);return Text;
    };
    Label(TEXT("ResultEyebrow"),TEXT("停  车  场  /  行  动  结  算"),250,23,FLinearColor(0.54f,0.60f,0.55f));
    Label(TEXT("ResultTitle"),Won?TEXT("逃生成功"):TEXT("行动失败"),328,76,Accent);
    Label(TEXT("ResultSubtitle"),Won?TEXT("你们成功离开了停车场。"):TEXT("队伍全员倒地，未能完成撤离。"),470,29,FLinearColor(0.84f,0.87f,0.83f));
    auto* Rule=WidgetTree->ConstructWidget<UImage>();Rule->SetBrush(FSlateColorBrush(Accent));Place(Rule,650,567,620,2);
    int32 Inserted=0;for(TActorIterator<ABRGarageKeyManager> Manager(GetWorld());Manager;++Manager){Inserted=Manager->GetInsertedKeys();break;}
    Label(TEXT("ResultDetails"),FString::Printf(TEXT("插入钥匙  %d / 4       队伍  %d 人"),Inserted,State?State->PlayerArray.Num():1),605,31,FLinearColor::White);
    const bool NetworkReturn=GetWorld()->GetNetMode()==NM_Client;
    Label(TEXT("ResultFooter"),NetworkReturn?TEXT("即将全队返回大厅…"):TEXT("本次行动已结束"),715,23,FLinearColor(0.54f,0.60f,0.55f));
    if(!NetworkReturn)
    {
        auto* Button=WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(),TEXT("ReturnMenuButton"));
        FButtonStyle Style;Style.SetNormal(FSlateColorBrush(FLinearColor(0.14f,0.18f,0.13f)));Style.SetHovered(FSlateColorBrush(FLinearColor(0.25f,0.31f,0.19f)));Style.SetPressed(Style.Hovered);Button->SetStyle(Style);
        auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(NSLOCTEXT("Backrooms","ResultReturnMenu","返回主菜单"));Text->SetFont(FSlateFontInfo(Font,26,TEXT("Regular")));Text->SetJustification(ETextJustify::Center);
        Button->AddChild(Text);Place(Button,780,802,360,68);Button->OnClicked.AddDynamic(this,&ThisClass::ReturnToMenu);
    }
}
void UBRRoundResultWidget::ReturnToMenu()
{if(auto* Controller=Cast<ABRMenuPlayerController>(GetOwningPlayer()))Controller->BRLeave();}
