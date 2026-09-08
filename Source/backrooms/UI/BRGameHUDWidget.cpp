#include "UI/BRGameHUDWidget.h"
#include "Rendering/DrawElements.h"
#include "UI/BRMenuPlayerController.h"
#include "Styling/CoreStyle.h"
#include "Core/BRGameState.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRStaminaComponent.h"
#include "Player/BRDownedComponent.h"
#include "Interaction/BRInteractionComponent.h"
#include "Interaction/BRInteractable.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "World/BRSupplyPickup.h"
#include "World/BRGarageKeyPickup.h"
#include "World/BRGarageKeySocket.h"
#include "World/BRGarageKeyManager.h"
#include "EngineUtils.h"

namespace
{
    bool IsPickup(const AActor* Actor)
    { return Cast<ABRSupplyPickup>(Actor) || Cast<ABRGarageKeyPickup>(Actor); }
}

void UBRGameHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();
    for(auto* Text:{KeyStatus.Get(),SanityStatus.Get(),RoundStatus.Get()})
        if(Text){Text->SetShadowColorAndOpacity(FLinearColor::Black);Text->SetShadowOffset(FVector2D(1.5f,1.5f));}
    if (!InteractionHint) return;
    InteractionHint->SetAutoWrapText(false);
    InteractionHint->SetShadowColorAndOpacity(FLinearColor::Black);
    InteractionHint->SetShadowOffset(FVector2D(1.5f,1.5f));
    InteractionHint->SetVisibility(ESlateVisibility::Collapsed);
    if (auto* HintSlot = Cast<UCanvasPanelSlot>(InteractionHint->Slot))
    {
        HintSlot->SetAnchors(FAnchors(0,0));
        HintSlot->SetAlignment(FVector2D(0.5f,1.f));
        HintSlot->SetAutoSize(true);
    }
}

void UBRGameHUDWidget::UpdatePickupPrompt()
{
    if (!InteractionHint) return;
    InteractionHint->SetVisibility(ESlateVisibility::Collapsed);
    auto* PC = Cast<ABRMenuPlayerController>(GetOwningPlayer());
    auto* Player = Cast<ABRPlayerCharacter>(GetOwningPlayerPawn());
    if (!PC || PC->IsInventoryOpen() || !Player || Player->GetDownedComponent()->IsDowned() || Player->GetCurrentHideSpot()) return;
    auto* Target = Player->GetInteractionComponent()->FindFocusedInteractable();
    const bool Socket=Cast<ABRGarageKeySocket>(Target)!=nullptr;
    if (!IsValid(Target) || (!IsPickup(Target) && !Socket) || Target->IsHidden() || (!Socket && !IBRInteractable::Execute_CanInteract(Target,Player))) return;

    FVector Origin, Extent;
    Target->GetActorBounds(false,Origin,Extent);
    FVector2D ScreenPosition;
    if (!UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC,Origin+FVector(0,0,Extent.Z+8.f),ScreenPosition,true)) return;
    auto* HintSlot = Cast<UCanvasPanelSlot>(InteractionHint->Slot);
    auto* Parent = InteractionHint->GetParent();
    if (!HintSlot || !Parent) return;
    // Convert through the viewport and canvas geometries so DPI, window size and
    // the HUD's 1920x1080 ScaleBox cannot detach the prompt from its world item.
    const FVector2D Absolute = UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(PC).LocalToAbsolute(ScreenPosition);
    const FVector2D Position = Parent->GetCachedGeometry().AbsoluteToLocal(Absolute)-FVector2D(0,12);
    HintSlot->SetPosition(Position);
    InteractionHint->SetText(IBRInteractable::Execute_GetInteractionText(Target,Player));
    InteractionHint->SetVisibility(ESlateVisibility::HitTestInvisible);
}

bool UBRGameHUDWidget::IsPickupPromptVisible() const
{ return InteractionHint && InteractionHint->IsVisible() && IsVisible(); }

bool UBRGameHUDWidget::ShouldDrawInteractionRing() const
{
    auto* Player = Cast<ABRPlayerCharacter>(GetOwningPlayerPawn());
    if (!Player || Player->GetDownedComponent()->IsDowned()) return false;
    auto* Target = Player->GetInteractionComponent()->FindFocusedInteractable();
    return IsValid(Target) && !IsPickup(Target) && !Cast<ABRGarageKeySocket>(Target) && IBRInteractable::Execute_CanInteract(Target,Player);
}

void UBRGameHUDWidget::NativeTick(const FGeometry& Geometry, float DeltaTime)
{
    Super::NativeTick(Geometry,DeltaTime);
    UpdatePickupPrompt();
    if (FPlatformTime::Seconds() < NextUpdate) return; NextUpdate = FPlatformTime::Seconds() + 0.1;
    auto* GS = GetWorld()->GetGameState<ABRGameState>(); auto* Character = Cast<ABRPlayerCharacter>(GetOwningPlayerPawn());
    if (!GS) return;
    if(SanityStatus && Character){auto* Bag=Character->GetInventoryComponent();SanityStatus->SetText(FText::FromString(FString::Printf(TEXT("SAN %.0f / 100\n[Tab] 背包与快捷栏 %d / 12"),Bag->GetSanity(),Bag->GetUsedSlots())));SanityStatus->SetColorAndOpacity(Bag->GetSanity()<25?FLinearColor(1,0.3f,0.2f,1):FLinearColor::White);}
    FString Inventory=FString::Printf(TEXT("钥匙  %d / %d     队伍 %d 人"),GS->GetCompletedObjectives(),GS->GetTotalObjectives(),GS->PlayerArray.Num());
    for(TActorIterator<ABRGarageKeyManager> Manager(GetWorld());Manager;++Manager)
    {
        if(Manager->UsesKeySockets())
        {
            Inventory=FString::Printf(TEXT("已收集钥匙 %d / 4     已插入 %d / 4     队伍 %d 人"),Manager->GetCollectedKeys(),Manager->GetInsertedKeys(),GS->PlayerArray.Num());
            if(Manager->AreAllKeysInserted())Inventory+=TEXT("\n中央门已打开 · 进入门后区域即可撤离");
            else if(Manager->GetCollectedKeys()==4)Inventory+=TEXT("\n前往中央房间，按E依次插入四把钥匙");
        }
        break;
    }
    if(Character && Character->HasFlashlight())Inventory+=FString::Printf(TEXT("\n[F] 手电 %s %.0f%%    [R] 换电池 · 备用 %d"),Character->IsFlashlightOn()?TEXT("开"):TEXT("关"),Character->GetBatteryCharge(),Character->GetSpareBatteries());
    if (Character)
    {
        const auto* Stamina = Character->GetStaminaComponent();
        Inventory += Stamina->HasUnlimitedStamina() ? TEXT("\n耐力 ∞ · 被追击中") :
            FString::Printf(TEXT("\n耐力 %.0f / %.0f%s"), Stamina->GetStamina(), Stamina->GetMaxStamina(),
                Stamina->IsExhausted() ? TEXT(" · 恢复中") : TEXT(""));
    }
    KeyStatus->SetText(FText::FromString(Inventory));
    FString Result;
    if (Character)
    {
        if (Character->GetDownedComponent()->IsDowned()) Result = TEXT("你已倒地，等待队友救援");

    }
    if (GS->GetLevelPhase()==EBRLevelPhase::Completed) Result=TEXT("逃生成功\n即将返回大厅");
    if (GS->GetLevelPhase()==EBRLevelPhase::Failed) Result=TEXT("队伍全员倒地\n即将返回大厅");
    RoundStatus->SetText(FText::FromString(Result));
}

bool UBRGameHUDWidget::HasInteractableFocus() const
{auto* P=Cast<ABRPlayerCharacter>(GetOwningPlayerPawn());if(!P || P->GetDownedComponent()->IsDowned())return false;
 auto* A=P->GetInteractionComponent()->FindFocusedInteractable();return A && IBRInteractable::Execute_CanInteract(A,P);}
int32 UBRGameHUDWidget::NativePaint(const FPaintArgs& A,const FGeometry& G,const FSlateRect& C,FSlateWindowElementList& E,int32 L,const FWidgetStyle& S,bool B) const
{
 int32 Layer=Super::NativePaint(A,G,C,E,L,S,B);auto* PC=Cast<ABRMenuPlayerController>(GetOwningPlayer());auto* P=Cast<ABRPlayerCharacter>(GetOwningPlayerPawn());
 if(!PC || PC->IsInventoryOpen() || !P || P->GetDownedComponent()->IsDowned())return Layer;
 const FVector2D Center=G.GetLocalSize()*0.5f;
 TArray<FVector2D> Points;const bool Focus=ShouldDrawInteractionRing();const float R=Focus?5.5f:0.7f;
 for(int32 I=0;I<=32;++I){const float T=2*PI*I/32;Points.Add(Center+FVector2D(FMath::Cos(T)*R,FMath::Sin(T)*R));}
 FSlateDrawElement::MakeLines(E,++Layer,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,FLinearColor(1,1,1,0.9f),true,Focus?1.25f:1.5f);return Layer;
}
