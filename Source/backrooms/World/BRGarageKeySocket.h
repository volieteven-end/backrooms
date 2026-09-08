#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/BRInteractable.h"
#include "BRGarageKeySocket.generated.h"

class ABRGarageKeyManager;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class BACKROOMS_API ABRGarageKeySocket : public AActor, public IBRInteractable
{
    GENERATED_BODY()
public:
    ABRGarageKeySocket();
    virtual void BeginPlay() override;
    virtual bool CanInteract_Implementation(APawn* Pawn) const override;
    virtual FText GetInteractionText_Implementation(APawn* Pawn) const override;
    virtual void Interact_Implementation(APawn* Pawn) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
    bool IsInserted() const { return bInserted; }
    bool IsInsertedKeyVisible() const;
    void MarkInserted();

    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Backrooms|Key Socket") TObjectPtr<ABRGarageKeyManager> KeyManager;
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Backrooms|Key Socket", meta=(ClampMin="1",ClampMax="4")) int32 SocketNumber=1;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Backrooms|Key Socket") TObjectPtr<UStaticMeshComponent> Plate;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Backrooms|Key Socket") TObjectPtr<UStaticMeshComponent> Keyhole;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Backrooms|Key Socket") TObjectPtr<UStaticMeshComponent> InsertedKey;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Backrooms|Key Socket") TObjectPtr<UBoxComponent> InteractionTarget;
private:
    UPROPERTY(ReplicatedUsing=OnRep_Inserted) bool bInserted=false;
    UFUNCTION() void OnRep_Inserted();
    void ApplyVisualState();
    bool bObservedInserted=false;
};
