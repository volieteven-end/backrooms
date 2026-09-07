#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/DragDropOperation.h"
#include "BRInventoryWidget.generated.h"
class UBRInventoryComponent;
UCLASS()
class BACKROOMS_API UBRInventoryDragOperation : public UDragDropOperation
{
 GENERATED_BODY()
public:
 UPROPERTY() TObjectPtr<UBRInventoryComponent> Inventory;
 UPROPERTY() TArray<FGuid> Ids;
 UPROPERTY() int32 From=INDEX_NONE;
};
UCLASS()
class BACKROOMS_API UBRInventoryWidget : public UUserWidget
{
 GENERATED_BODY()
public:
 virtual void NativeConstruct() override;
 virtual void NativeTick(const FGeometry& G,float DT) override;
 virtual FReply NativeOnKeyDown(const FGeometry& G,const FKeyEvent& E) override;
 virtual FReply NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override;
 virtual FReply NativeOnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override;
 virtual void NativeOnDragDetected(const FGeometry& G,const FPointerEvent& E,UDragDropOperation*& Operation) override;
 virtual bool NativeOnDrop(const FGeometry& G,const FDragDropEvent& E,UDragDropOperation* Operation) override;
 void Select(int32 I);
 UFUNCTION() void Use();
 UFUNCTION() void Drop();
 UFUNCTION() void Close();
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Inventory Art") TObjectPtr<UTexture2D> FlashlightIcon;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Inventory Art") TObjectPtr<UTexture2D> AlmondWaterIcon;
private:
 void Refresh();
 int32 SlotAt(FVector2D Position) const;
 UBRInventoryComponent* Inventory() const;
 int32 FocusedSlot=INDEX_NONE,PressedSlot=INDEX_NONE;
 double NextRefresh=0;
};
