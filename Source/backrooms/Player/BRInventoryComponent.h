#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BRInventoryComponent.generated.h"
UENUM(BlueprintType)
enum class EBRInventoryItem : uint8 { Empty, Flashlight, Battery, AlmondWater };
USTRUCT(BlueprintType)
struct FBRInventorySlot
{
    GENERATED_BODY()
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) EBRInventoryItem Item=EBRInventoryItem::Empty;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly) FGuid InstanceId;
};
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FBRInventoryChanged);
UCLASS(ClassGroup=(Backrooms),meta=(BlueprintSpawnableComponent))
class BACKROOMS_API UBRInventoryComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBRInventoryComponent();
    static constexpr int32 BackpackCapacity=9;
    static constexpr int32 MainHandSlot=9;
    static constexpr int32 PocketStart=10;
    static constexpr int32 Capacity=12;
    virtual void TickComponent(float DeltaTime,ELevelTick TickType,FActorComponentTickFunction* Function) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    UFUNCTION(BlueprintPure) const TArray<FBRInventorySlot>& GetSlots() const {return Slots;}
    UFUNCTION(BlueprintPure) float GetSanity() const {return Sanity;}
    UFUNCTION(BlueprintPure) int32 GetSelectedSlot() const {return SelectedSlot;}
    UFUNCTION(BlueprintPure) EBRInventoryItem GetEquippedItem() const {return EquippedItem;}
    UFUNCTION(BlueprintPure) int32 Count(EBRInventoryItem Item) const;
    UFUNCTION(BlueprintPure) int32 GetUsedSlots() const;
    UFUNCTION(BlueprintPure) bool CanAdd(EBRInventoryItem Item) const;
    // Called by server-validated world interactions only; clients never choose grant types.
    bool AddItem(EBRInventoryItem Item);
    bool ConsumeBattery();
    void SelectFirst(EBRInventoryItem Item);
    UFUNCTION(BlueprintCallable) void SelectSlot(int32 Index);
    UFUNCTION(BlueprintCallable) void SwapSlots(int32 From,int32 To);
    UFUNCTION(Server,Reliable) void ServerSwapSlots(int32 From,int32 To,FGuid FromId,FGuid ToId);
    UFUNCTION(BlueprintCallable) void StowHeld();
    UFUNCTION(BlueprintCallable) void UseSelected();
    UFUNCTION(BlueprintCallable) void DropSelected();
    UFUNCTION(Server,Reliable) void ServerSelectSlot(int32 Index,FGuid ExpectedId);
    UFUNCTION(Server,Reliable) void ServerUseSlot(int32 Index,FGuid ExpectedId);
    UFUNCTION(Server,Reliable) void ServerDropSlot(int32 Index,FGuid ExpectedId);
    UPROPERTY(BlueprintAssignable) FBRInventoryChanged OnInventoryChanged;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Sanity",meta=(ClampMin="0",ClampMax="1")) float SanityDrainPerSecond=0.1f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Sanity",meta=(ClampMin="1",ClampMax="100")) float AlmondWaterRestore=35.0f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Sanity") bool bPauseDrainWhileHiding=true;
    static FText ItemName(EBRInventoryItem Item);
private:
    UPROPERTY(ReplicatedUsing=OnRep_Inventory) TArray<FBRInventorySlot> Slots;
    UPROPERTY(ReplicatedUsing=OnRep_Inventory) float Sanity=100;
    UPROPERTY(ReplicatedUsing=OnRep_Inventory) int32 SelectedSlot=INDEX_NONE;
    UPROPERTY(ReplicatedUsing=OnRep_Inventory) EBRInventoryItem EquippedItem=EBRInventoryItem::Empty;
    UFUNCTION() void OnRep_Inventory();
    bool CanOperate() const;
    bool Matches(int32 Index,FGuid Id) const;
    void RemoveAt(int32 Index);
    void Changed();
    float SanityTick=0;
    double NextUseTime=0;
};
