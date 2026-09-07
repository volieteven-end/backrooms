#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "Interaction/BRInteractable.h"
#include "BRPlayerCharacter.generated.h"

class UBRInventoryComponent;
class UBRFirstPersonArmsComponent;
class UBRInteractionComponent;
class ABRHideSpot;
class UAnimSequence;
class UBRDownedComponent;
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class UStaticMeshComponent;
class USpotLightComponent;

UCLASS()
class BACKROOMS_API ABRPlayerCharacter : public ACharacter, public IBRInteractable
{
	GENERATED_BODY()

public:
	ABRPlayerCharacter();
    virtual void CalcCamera(float DeltaTime,FMinimalViewInfo& OutResult) override;
    virtual void GetActorEyesViewPoint(FVector& Location, FRotator& Rotation) const override;
    bool GrantFlashlight(float Charge=100);
    bool GrantAlmondWater();
    void RefreshInventoryEquipment();
    UFUNCTION(Client,Reliable) void ClientPlayArmsAction(uint8 Action);
    UFUNCTION(BlueprintPure) UBRInventoryComponent* GetInventoryComponent() const {return InventoryComponent;}
    UFUNCTION(BlueprintPure) UBRFirstPersonArmsComponent* GetArmsAnimationComponent() const {return ArmsAnimation;}
    bool GrantBattery();
    UFUNCTION(BlueprintPure,Category="Backrooms|Equipment") bool HasFlashlight() const {return bHasFlashlight;}
    UFUNCTION(BlueprintPure,Category="Backrooms|Equipment") bool IsFlashlightOn() const {return bFlashlightOn;}
    UFUNCTION(BlueprintPure,Category="Backrooms|Equipment") float GetBatteryCharge() const {return BatteryCharge;}
    UFUNCTION(BlueprintPure,Category="Backrooms|Equipment") int32 GetSpareBatteries() const {return SpareBatteries;}
    void ToggleFlashlight();
    void ReplaceBattery();
    virtual void Tick(float DeltaTime) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UFUNCTION(BlueprintPure) ABRHideSpot* GetCurrentHideSpot() const { return CurrentHideSpot; }
    void SetCurrentHideSpot(ABRHideSpot* Spot);

	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual bool CanInteract_Implementation(APawn* InstigatorPawn) const override;
	virtual FText GetInteractionText_Implementation(APawn* InstigatorPawn) const override;
	virtual void Interact_Implementation(APawn* InstigatorPawn) override;

	UFUNCTION(BlueprintPure, Category = "Backrooms|Player")
	UBRDownedComponent* GetDownedComponent() const { return DownedComponent; }

	UFUNCTION(BlueprintPure, Category = "Backrooms|Interaction")
	UBRInteractionComponent* GetInteractionComponent() const { return InteractionComponent; }

protected:
	virtual void BeginPlay() override;

    UPROPERTY(EditDefaultsOnly,BlueprintReadWrite,Category="Backrooms|Camera",meta=(ClampMin="0.1",ClampMax="5")) float FirstPersonNearClipDistance=1.0f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Camera")
	TObjectPtr<UCameraComponent> FirstPersonCamera;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|First Person") TObjectPtr<USkeletalMeshComponent> FirstPersonArms;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Equipment") TObjectPtr<UStaticMeshComponent> FirstPersonFlashlight;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Equipment") TObjectPtr<UStaticMeshComponent> WorldFlashlight;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Equipment") TObjectPtr<USpotLightComponent> FlashlightLight;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Equipment",meta=(ClampMin="0")) float FlashlightLumens=60.0f;
    UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Backrooms|Equipment",meta=(ClampMin="0")) float BatteryDrainPerSecond=0.25f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Components")
	TObjectPtr<UBRInteractionComponent> InteractionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Components")
	TObjectPtr<UBRDownedComponent> DownedComponent;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Components") TObjectPtr<UBRInventoryComponent> InventoryComponent;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Components") TObjectPtr<UBRFirstPersonArmsComponent> ArmsAnimation;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Equipment") TObjectPtr<UStaticMeshComponent> FirstPersonCan;
    UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Backrooms|Equipment") TObjectPtr<USceneComponent> ItemGrip;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Input")
	TObjectPtr<UInputAction> InteractAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Movement")
	float WalkSpeed = 350.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Backrooms|Movement")
	float SprintSpeed = 600.0f;

	UPROPERTY(ReplicatedUsing = OnRep_IsSprinting, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|Movement")
	bool bIsSprinting = false;

	UFUNCTION()
	void OnRep_IsSprinting();

	UFUNCTION(Server, Reliable)
	void ServerSetSprinting(bool bNewSprinting);

private:
    UPROPERTY() TObjectPtr<UAnimSequence> ArmsIdleAnimation;
    UPROPERTY(ReplicatedUsing=OnRep_Equipment) bool bHasFlashlight=false;
    UPROPERTY(ReplicatedUsing=OnRep_Equipment) bool bFlashlightOn=false;
    UPROPERTY(Replicated) float BatteryCharge=100;
    UPROPERTY(Replicated) int32 SpareBatteries=0;
    UFUNCTION() void OnRep_Equipment();
    UFUNCTION(Server,Reliable) void ServerToggleFlashlight();
    UFUNCTION(Server,Reliable) void ServerReplaceBattery();
    void TickEquipment(float DeltaTime);
    double NextEquipmentInput=0;
    float BatteryTickAccumulator=0;
    UPROPERTY(ReplicatedUsing=OnRep_CurrentHideSpot) TObjectPtr<ABRHideSpot> CurrentHideSpot;
    UFUNCTION() void OnRep_CurrentHideSpot();
    UPROPERTY() TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> RunAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> CrouchIdleAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> CrouchWalkAnimation;
    UPROPERTY() TObjectPtr<UAnimSequence> ActiveAnimation;
	void MoveEnhanced(const FInputActionValue& Value);
	void LookEnhanced(const FInputActionValue& Value);
	void MoveForwardLegacy(float Value);
	void MoveRightLegacy(float Value);
	void StartSprint();
	void StopSprint();
	void ToggleCrouch();
	void TryInteract();
	void ApplySprintState();
};
