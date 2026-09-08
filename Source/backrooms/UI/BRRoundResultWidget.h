#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "BRRoundResultWidget.generated.h"

UCLASS()
class BACKROOMS_API UBRRoundResultWidget : public UUserWidget
{
    GENERATED_BODY()
protected:
    virtual void NativeOnInitialized() override;
private:
    UFUNCTION() void ReturnToMenu();
};
