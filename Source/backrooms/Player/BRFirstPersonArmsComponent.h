#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BRFirstPersonArmsComponent.generated.h"
class UAnimSequence;class UBlendSpace;class USkeletalMeshComponent;
UCLASS(ClassGroup=(Backrooms),meta=(BlueprintSpawnableComponent))
class BACKROOMS_API UBRFirstPersonArmsComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UBRFirstPersonArmsComponent();
    void Update(float DT,USkeletalMeshComponent* Mesh);
    void PlayAction(uint8 Action);
    bool WantsVisibleArms() const;
    bool IsDrinking() const {return ActiveAction==5;}
    UFUNCTION(BlueprintPure) FName GetAnimationState() const {return State;}
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") FVector EmptyOffset=FVector(-7,0,-67.5);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") FVector EquippedOffset=FVector(3,0,-67.5);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") FVector DrinkOffset=FVector(3,0,-67.5);
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms",meta=(ClampMin="0",ClampMax="2")) float SwayStrength=1;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> Idle;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> Walk;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> Run;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UBlendSpace> EmptyLocomotion;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> FlashlightEquip;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> FlashlightHold;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> FlashlightToggle;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> Grab;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> Stow;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> CanEquip;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> CanHold;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Arms") TObjectPtr<UAnimSequence> Drink;
private:
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> Active;
    FName State=NAME_None;
    uint8 PendingAction=0,ActiveAction=0;
    float Phase=0,Motion=0;
    FRotator PreviousView=FRotator::ZeroRotator;
    bool bInitialized=false;
    bool bUsingLocomotion=false;
};
