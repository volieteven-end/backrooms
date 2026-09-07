#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/BRInteractable.h"
#include "BRGarageDoor.generated.h"

class UBoxComponent;

UCLASS()
class BACKROOMS_API ABRGarageDoor : public AActor, public IBRInteractable
{
	GENERATED_BODY()

public:
	ABRGarageDoor();
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	bool OwnsPanel(const AActor* Panel) const;
	bool SetDoorOpen(bool bNewOpen, APawn* InstigatorPawn = nullptr);
	float GetOpenAlpha() const { return OpenAlpha; }
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Door Motion", meta=(ClampMin="0.15",ClampMax="5")) float OpenDuration = 0.7f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Door Motion") float OpenAngle = 95.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Door Motion") bool bOpenAwayFromPlayer = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Door Motion") bool bDoubleLeaf = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Door Motion") bool bSliding = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Door Motion") FVector SlideOffset = FVector(30,0,0);

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionText_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Garage")
	void SetUnlockable(bool bNewUnlockable);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backrooms|Garage")
	FName ControlledActorTag = NAME_None;

	UFUNCTION(BlueprintPure, Category = "Backrooms|Garage")
	bool IsOpen() const { return bOpen; }

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	TObjectPtr<UBoxComponent> InteractionBox;

	UPROPERTY(ReplicatedUsing = OnRep_DoorState, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	bool bUnlockable = false;

	UPROPERTY(ReplicatedUsing = OnRep_DoorState, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Garage")
	bool bOpen = false;

	UFUNCTION()
	void OnRep_DoorState();

protected:
	void ApplyDoorState();
    bool bAudioObservedOpen = false;
private:
	void CachePanels();
	bool IsClosingBlocked() const;
	UPROPERTY(ReplicatedUsing=OnRep_DoorState) float OpenDirection = 1.0f;
	TArray<TWeakObjectPtr<AActor>> Panels;
	TArray<FTransform> ClosedTransforms;
	float OpenAlpha = 0;
	double NextInteractionTime = 0;
};
