#pragma once
#include "CoreMinimal.h"
#include "World/BRGarageDoor.h"
#include "BRGarageExitDoor.generated.h"
class ABRGarageKeyManager;

UCLASS()
class BACKROOMS_API ABRGarageExitDoor : public ABRGarageDoor
{
    GENERATED_BODY()
public:
    ABRGarageExitDoor();
    virtual bool CanInteract_Implementation(APawn* Pawn) const override { return false; }
    virtual void Interact_Implementation(APawn* Pawn) override {}
    bool OpenAfterKeysInserted();
    bool IsPassageOpen() const { return IsOpen() && GetOpenAlpha()>=0.98f; }
    UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category="Backrooms|Exit") TObjectPtr<ABRGarageKeyManager> KeyManager;
};
