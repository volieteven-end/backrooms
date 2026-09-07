#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/BRInteractable.h"
#include "BRHideSpot.generated.h"

class UBoxComponent;
class ABRLootCabinet;

UCLASS()
class BACKROOMS_API ABRHideSpot : public AActor, public IBRInteractable
{
	GENERATED_BODY()

public:
	ABRHideSpot();
    virtual void BeginPlay() override;
    UPROPERTY(EditInstanceOnly,BlueprintReadWrite,Category="Backrooms|Hiding") TObjectPtr<ABRLootCabinet> Cabinet;
    void ReleaseOccupant();
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionText_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	UFUNCTION(BlueprintPure, Category = "Backrooms|Hiding")
	bool IsOccupied() const { return IsValid(Occupant); }

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backrooms|Hiding")
	FVector ExitOffset = FVector(130.0f, 0.0f, 0.0f);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Hiding")
	TObjectPtr<UBoxComponent> InteractionBox;

private:
    void Enter(APawn* Pawn);
    void Exit(APawn* Pawn);
    UPROPERTY(Replicated) TObjectPtr<APawn> Occupant;
    FVector EntryLocation = FVector::ZeroVector;
};
