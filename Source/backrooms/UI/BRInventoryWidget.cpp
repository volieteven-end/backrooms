#include "UI/BRInventoryWidget.h"
#include "Components/SizeBox.h"
#include "UI/BRMenuPlayerController.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRPlayerCharacter.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Engine/Texture2D.h"
UBRInventoryComponent* UBRInventoryWidget::Inventory() const
{auto* P=Cast<ABRPlayerCharacter>(GetOwningPlayerPawn());return P?P->GetInventoryComponent():nullptr;}
void UBRInventoryWidget::NativeConstruct()
{
    Super::NativeConstruct();SetIsFocusable(true);
#define BIND(N,F) if(auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT(N))))B->OnClicked.AddDynamic(this,&ThisClass::F);
    BIND("UseButton",Use);BIND("DropButton",Drop);BIND("CloseButton",Close);
#undef BIND
    if(auto* Inv=Inventory())FocusedSlot=Inv->GetSelectedSlot();Refresh();
}
void UBRInventoryWidget::NativeTick(const FGeometry& G,float DT)
{Super::NativeTick(G,DT);if(FPlatformTime::Seconds()>=NextRefresh){NextRefresh=FPlatformTime::Seconds()+0.1;Refresh();}}
void UBRInventoryWidget::Refresh()
{
    auto* Inv=Inventory();if(!Inv)return;
    auto Text=[this](const FString& N,const FString& V){if(auto* T=Cast<UTextBlock>(WidgetTree->FindWidget(FName(*N))))T->SetText(FText::FromString(V));};
    Text(TEXT("CapacityText"),TEXT("背包"));
    Text(TEXT("SanityText"),FString::Printf(TEXT("SAN  %.0f / 100"),Inv->GetSanity()));
    Text(TEXT("SanityState"),Inv->GetSanity()<25?TEXT("精神紧绷 · 喝杏仁水恢复"):Inv->GetSanity()<60?TEXT("保持警惕"):TEXT("精神稳定"));
    if(auto* Bar=Cast<UProgressBar>(WidgetTree->FindWidget(TEXT("SanityBar"))))
    {Bar->SetPercent(Inv->GetSanity()/100.f);Bar->SetFillColorAndOpacity(Inv->GetSanity()<25?FLinearColor(0.75f,0.16f,0.1f,1):FLinearColor(0.32f,0.4f,0.42f,0.75f));}
    if(auto* Brain=Cast<UImage>(WidgetTree->FindWidget(TEXT("SanityBrain"))))
    {const TCHAR* Name=Inv->GetSanity()<25?TEXT("Level_3"):Inv->GetSanity()<60?TEXT("Level_2"):TEXT("Level_1");
        auto* Tex=LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/UI/Inventory/Art/"))+Name+TEXT(".")+Name));if(Tex)Brain->SetBrushFromTexture(Tex);}
    const auto& Slots=Inv->GetSlots();
    for(int32 I=0;I<UBRInventoryComponent::Capacity;++I)
    {
        const EBRInventoryItem Item=Slots.IsValidIndex(I)?Slots[I].Item:EBRInventoryItem::Empty;
        Text(FString::Printf(TEXT("ItemName%d"),I),Item==EBRInventoryItem::Empty?TEXT(""):UBRInventoryComponent::ItemName(Item).ToString());
        if(auto* B=WidgetTree->FindWidget(FName(*FString::Printf(TEXT("BatteryIcon%d"),I))))B->SetVisibility(Item==EBRInventoryItem::Battery?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
        if(auto* B=WidgetTree->FindWidget(FName(*FString::Printf(TEXT("SelectedMark%d"),I))))B->SetVisibility(I==FocusedSlot?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);
        if(auto* Icon=Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("ItemIcon%d"),I)))))
        {auto* T=Item==EBRInventoryItem::Flashlight?FlashlightIcon.Get():Item==EBRInventoryItem::AlmondWater?AlmondWaterIcon.Get():nullptr;
            Icon->SetVisibility(T?ESlateVisibility::HitTestInvisible:ESlateVisibility::Hidden);if(T)Icon->SetBrushFromTexture(T);}
        if(auto* Button=Cast<UButton>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("Slot%d"),I)))))
            Button->SetBackgroundColor(I==FocusedSlot?FLinearColor(0.85f,0.95f,0.95f,1):FLinearColor::White);
    }
    const auto Item=Slots.IsValidIndex(FocusedSlot)?Slots[FocusedSlot].Item:EBRInventoryItem::Empty;
    if(auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("UseButton"))))B->SetIsEnabled(Item!=EBRInventoryItem::Empty);
    if(auto* B=Cast<UButton>(WidgetTree->FindWidget(TEXT("DropButton"))))B->SetIsEnabled(Item!=EBRInventoryItem::Empty);

}
void UBRInventoryWidget::Select(int32 I){FocusedSlot=UBRInventoryComponent::MainHandSlot;if(auto* Inv=Inventory())Inv->SelectSlot(I);Refresh();}
void UBRInventoryWidget::Use(){if(auto* Inv=Inventory())if(Inv->GetSlots().IsValidIndex(FocusedSlot))Inv->ServerUseSlot(FocusedSlot,Inv->GetSlots()[FocusedSlot].InstanceId);}
void UBRInventoryWidget::Drop(){if(auto* Inv=Inventory())if(Inv->GetSlots().IsValidIndex(FocusedSlot))Inv->ServerDropSlot(FocusedSlot,Inv->GetSlots()[FocusedSlot].InstanceId);}
void UBRInventoryWidget::Close(){if(auto* PC=Cast<ABRMenuPlayerController>(GetOwningPlayer()))PC->ToggleInventory();}
FReply UBRInventoryWidget::NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E)
{
 if(E.GetKey()==EKeys::Tab || E.GetKey()==EKeys::Escape){Close();return FReply::Handled();}
 if(E.GetKey()==EKeys::E){Use();return FReply::Handled();}if(E.GetKey()==EKeys::G){Drop();return FReply::Handled();}
 return Super::NativeOnKeyDown(G,E);
}
int32 UBRInventoryWidget::SlotAt(FVector2D Position) const
{for(int32 I=0;I<UBRInventoryComponent::Capacity;++I)if(auto* W=WidgetTree->FindWidget(FName(*FString::Printf(TEXT("Slot%d"),I))))
 if(W->GetCachedGeometry().IsUnderLocation(Position))return I;return INDEX_NONE;}
FReply UBRInventoryWidget::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
 PressedSlot=SlotAt(E.GetScreenSpacePosition());FocusedSlot=PressedSlot;
 if(PressedSlot!=INDEX_NONE)
 {Refresh();if(E.GetEffectingButton()==EKeys::LeftMouseButton)return FReply::Handled().DetectDrag(TakeWidget(),EKeys::LeftMouseButton);
 if(E.GetEffectingButton()==EKeys::RightMouseButton){Use();return FReply::Handled();}}
 return Super::NativeOnMouseButtonDown(G,E);
}
FReply UBRInventoryWidget::NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)
{
 if(E.GetEffectingButton()==EKeys::LeftMouseButton && PressedSlot!=INDEX_NONE)
 {if(PressedSlot==SlotAt(E.GetScreenSpacePosition()))Select(PressedSlot);PressedSlot=INDEX_NONE;return FReply::Handled();}
 return Super::NativeOnMouseButtonUp(G,E);
}
void UBRInventoryWidget::NativeOnDragDetected(const FGeometry&,const FPointerEvent&,UDragDropOperation*& Operation)
{
 auto* Inv=Inventory();if(!Inv || !Inv->GetSlots().IsValidIndex(PressedSlot) || !Inv->GetSlots()[PressedSlot].InstanceId.IsValid())return;
 auto* Op=NewObject<UBRInventoryDragOperation>();Op->Inventory=Inv;Op->From=PressedSlot;for(const auto& S:Inv->GetSlots())Op->Ids.Add(S.InstanceId);
 auto* Image=NewObject<UImage>(Op);if(auto* Icon=Cast<UImage>(WidgetTree->FindWidget(FName(*FString::Printf(TEXT("ItemIcon%d"),PressedSlot)))))Image->SetBrush(Icon->GetBrush());
 auto* Size=NewObject<USizeBox>(Op);Size->SetWidthOverride(110);Size->SetHeightOverride(110);Size->AddChild(Image);Size->SetVisibility(ESlateVisibility::HitTestInvisible);
 Op->DefaultDragVisual=Size;Op->Pivot=EDragPivot::CenterCenter;Operation=Op;PressedSlot=INDEX_NONE;
}
bool UBRInventoryWidget::NativeOnDrop(const FGeometry&,const FDragDropEvent& E,UDragDropOperation* Operation)
{
 auto* Op=Cast<UBRInventoryDragOperation>(Operation);auto* Inv=Inventory();const int32 To=SlotAt(E.GetScreenSpacePosition());
 if(!Op || Op->Inventory!=Inv || !Op->Ids.IsValidIndex(To) || !Op->Ids.IsValidIndex(Op->From))return false;
 Inv->ServerSwapSlots(Op->From,To,Op->Ids[Op->From],Op->Ids[To]);FocusedSlot=To;Refresh();return true;
}
