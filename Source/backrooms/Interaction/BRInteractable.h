#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "BRInteractable.generated.h"

class APawn;

UINTERFACE(Blueprintable)
class BACKROOMS_API UBRInteractable : public UInterface
{
	GENERATED_BODY()
};

class BACKROOMS_API IBRInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Backrooms|Interaction")
	bool CanInteract(APawn* InstigatorPawn) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Backrooms|Interaction")
	FText GetInteractionText(APawn* InstigatorPawn) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Backrooms|Interaction")
	void Interact(APawn* InstigatorPawn);
};
