#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/BRInteractable.h"
#include "BRGarageKeyPickup.generated.h"

class ABRGarageKeyManager;
class UStaticMeshComponent;
class USphereComponent;

UCLASS()
class BACKROOMS_API ABRGarageKeyPickup : public AActor, public IBRInteractable
{
	GENERATED_BODY()

public:
	ABRGarageKeyPickup();
    void SetContainerLocked(bool Locked);

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionText_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Garage")
	void SetPickupActive(bool bNewActive);

	UFUNCTION(BlueprintPure, Category = "Backrooms|Garage")
	bool IsCollected() const { return bCollected; }

	UFUNCTION(BlueprintPure, Category = "Backrooms|Garage")
	bool IsPickupActive() const { return bPickupActive && !bCollected && !bContainerLocked; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(ReplicatedUsing = OnRep_PickupState, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	bool bPickupActive = true;

	UPROPERTY(ReplicatedUsing = OnRep_PickupState, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	bool bCollected = false;

	UFUNCTION()
	void OnRep_PickupState();

private:
    UPROPERTY(VisibleAnywhere) TObjectPtr<USphereComponent> PickupTarget;
    UPROPERTY(ReplicatedUsing=OnRep_PickupState) bool bContainerLocked=false;
	void ApplyPickupState();
    bool bAudioObservedCollected = false;
};
