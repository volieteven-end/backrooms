#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/BRInteractable.h"
#include "BRObjectiveActor.generated.h"

class UStaticMeshComponent;

UCLASS()
class BACKROOMS_API ABRObjectiveActor : public AActor, public IBRInteractable
{
	GENERATED_BODY()

public:
	ABRObjectiveActor();

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionText_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Objectives")
	void CompleteObjective(APawn* CompletingPawn);

	UFUNCTION(BlueprintPure, Category = "Backrooms|Objectives")
	bool IsCompleted() const { return bCompleted; }

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Objectives")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Backrooms|Objectives")
	FText InteractionText = NSLOCTEXT("Backrooms", "CompleteObjective", "Activate objective");

	UPROPERTY(ReplicatedUsing = OnRep_Completed, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Objectives")
	bool bCompleted = false;

	UFUNCTION()
	void OnRep_Completed();

	UFUNCTION(BlueprintImplementableEvent, Category = "Backrooms|Objectives")
	void OnObjectiveCompleted(APawn* CompletingPawn);

	UFUNCTION(BlueprintImplementableEvent, Category = "Backrooms|Objectives")
	void OnObjectiveStateChanged(bool bNowCompleted);
};
