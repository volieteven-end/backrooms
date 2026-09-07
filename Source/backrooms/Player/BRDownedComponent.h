#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BRDownedComponent.generated.h"

class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBRDownedStateChanged, bool, bIsDowned);

UCLASS(ClassGroup = (Backrooms), meta = (BlueprintSpawnableComponent))
class BACKROOMS_API UBRDownedComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UBRDownedComponent();

	UFUNCTION(BlueprintPure, Category = "Backrooms|Player")
	bool IsDowned() const { return bDowned; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Player")
	void Down();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|Player")
	void Revive(APawn* Reviver);

	UPROPERTY(BlueprintAssignable, Category = "Backrooms|Player")
	FBRDownedStateChanged OnDownedStateChanged;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	UPROPERTY(ReplicatedUsing = OnRep_Downed, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Player")
	bool bDowned = false;

	UFUNCTION()
	void OnRep_Downed();

private:
	void ApplyDownedState();
};
