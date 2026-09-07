#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/BRInteractable.h"
#include "BRSupplyPickup.generated.h"
class UStaticMeshComponent;
class USphereComponent;
UENUM(BlueprintType)
enum class EBRSupplyType : uint8 { Flashlight, Battery, AlmondWater };
UCLASS()
class BACKROOMS_API ABRSupplyPickup : public AActor, public IBRInteractable
{
    GENERATED_BODY()
public:
    ABRSupplyPickup();
    virtual void OnConstruction(const FTransform& Transform) override;
    virtual void BeginPlay() override;
    virtual bool CanInteract_Implementation(APawn* Pawn) const override;
    virtual FText GetInteractionText_Implementation(APawn* Pawn) const override;
    virtual void Interact_Implementation(APawn* Pawn) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    void SetContainerVisible(bool Visible);
    bool IsCollected() const {return bCollected;}
    UPROPERTY(EditAnywhere,ReplicatedUsing=OnRep_State,BlueprintReadOnly,Category="Backrooms|Item") EBRSupplyType SupplyType=EBRSupplyType::Flashlight;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Item",meta=(ClampMin="0",ClampMax="100")) float StoredCharge=100;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Item") TObjectPtr<UStaticMeshComponent> Mesh;
private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> PickupTarget;
    UPROPERTY(ReplicatedUsing=OnRep_State) bool bCollected=false;
    UPROPERTY(ReplicatedUsing=OnRep_State) bool bContainerVisible=true;
    UFUNCTION() void OnRep_State();
    void ApplyState();
    bool bObservedCollected=false;
};
