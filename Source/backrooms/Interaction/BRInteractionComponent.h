#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BRInteractionComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FBRFocusedInteractableChanged, AActor*, FocusedActor, FText, InteractionText);

UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UBRInteractionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBRInteractionComponent();

	UFUNCTION(BlueprintCallable, Category = "Backrooms|Interaction")
	void TryInteract();

	UFUNCTION(BlueprintCallable, Category = "Backrooms|Interaction")
	AActor* FindFocusedInteractable() const;
    AActor* ResolveHitTarget(AActor* HitActor) const;

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Interaction")
	FBRFocusedInteractableChanged OnFocusedInteractableChanged;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Backrooms|Interaction", meta = (ClampMin = "50.0"))
	float TraceDistance = 350.0f;

protected:
	UFUNCTION(Server, Reliable)
	void ServerInteract(AActor* TargetActor);

private:
    AActor* TraceTarget(const FVector& Start,const FVector& End,const APawn* OwnerPawn) const;
	bool IsTargetValidForInteraction(AActor* TargetActor) const;
};
