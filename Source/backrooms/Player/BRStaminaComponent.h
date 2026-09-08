#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BRStaminaComponent.generated.h"

/** Server-owned sprint resource. Any server-controlled entity can register a chase. */
UCLASS(ClassGroup=(Backrooms), meta=(BlueprintSpawnableComponent))
class BACKROOMS_API UBRStaminaComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBRStaminaComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;

    UFUNCTION(BlueprintPure, Category="Backrooms|Stamina") float GetStamina() const { return Stamina; }
    UFUNCTION(BlueprintPure, Category="Backrooms|Stamina") float GetMaxStamina() const { return FMath::Max(1.f, MaxStamina); }
    UFUNCTION(BlueprintPure, Category="Backrooms|Stamina") bool HasUnlimitedStamina() const { return bBeingChased; }
    UFUNCTION(BlueprintPure, Category="Backrooms|Stamina") bool CanSprint() const { return bBeingChased || (!bExhausted && Stamina > 0); }
    UFUNCTION(BlueprintPure, Category="Backrooms|Stamina") bool IsExhausted() const { return bExhausted && !bBeingChased; }

    // No client RPC: chase status comes from authoritative entity behavior, never player input.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Backrooms|Stamina")
    void SetChasedBy(AActor* Entity, bool bChasing);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Stamina", meta=(ClampMin="1")) float MaxStamina = 100;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Stamina", meta=(ClampMin="0")) float SprintDrainPerSecond = 20;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Stamina", meta=(ClampMin="0")) float RecoveryPerSecond = 25;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Stamina", meta=(ClampMin="0")) float RecoveryDelay = 1.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Backrooms|Stamina", meta=(ClampMin="1")) float SprintResumeThreshold = 20;

protected:
    virtual void BeginPlay() override;

private:
    UPROPERTY(ReplicatedUsing=OnRep_Stamina) float Stamina = 100;
    UPROPERTY(ReplicatedUsing=OnRep_Stamina) bool bExhausted = false;
    UPROPERTY(ReplicatedUsing=OnRep_Stamina) bool bBeingChased = false;
    UFUNCTION() void OnRep_Stamina();
    void RefreshChasers();
    TSet<TWeakObjectPtr<AActor>> Chasers;
    float RecoveryRemaining = 0;
};
