#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRGameHUDWidget.generated.h"
class UTextBlock;
UCLASS()
class BACKROOMS_API UBRGameHUDWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    bool HasInteractableFocus() const;
protected:
    virtual int32 NativePaint(const FPaintArgs& Args,const FGeometry& Geometry,const FSlateRect& Cull,FSlateWindowElementList& Elements,int32 Layer,const FWidgetStyle& Style,bool bEnabled) const override;
    virtual void NativeTick(const FGeometry& Geometry, float DeltaTime) override;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> KeyStatus;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> InteractionHint;
    UPROPERTY(meta=(BindWidget)) TObjectPtr<UTextBlock> RoundStatus;
    UPROPERTY(meta=(BindWidgetOptional)) TObjectPtr<UTextBlock> SanityStatus;
private:
    double NextUpdate = 0;
};
