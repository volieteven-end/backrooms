#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Core/BRFrameworkTypes.h"
#include "BREntityCharacter.generated.h"
class UAnimSequence;
class UAudioComponent;

UCLASS()
class BACKROOMS_API ABREntityCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABREntityCharacter();
    virtual void Tick(float DeltaSeconds) override;
    void PlayReplicatedAttack(APawn* Victim = nullptr);
    float GetAttackAnimationDuration() const;
    int32 GetAttackCueCount() const { return AttackCueCount; }
    /** Number of live roar cues received by this instance (also available on the server). */
    int32 GetRoarCueCount() const { return RoarCueCount; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Backrooms|AI")
	void SetEntityState(EBREntityState NewState, AActor* NewTarget = nullptr);

	UFUNCTION(BlueprintPure, Category = "Backrooms|AI")
	EBREntityState GetEntityState() const { return EntityState; }

	UFUNCTION(BlueprintPure, Category = "Backrooms|AI")
	AActor* GetTargetActor() const { return TargetActor; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|Animation") float MeshYawOffset = -90.0f;
    void ApplyFacingConfiguration();
    void UpdateRoar();
    UFUNCTION(NetMulticast, Unreliable) void MulticastPlayRoar();
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> RoarAudio;
    int32 RoarCueCount = 0;
    double NextRoarTime = 0;

	UPROPERTY(ReplicatedUsing = OnRep_EntityState, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|AI")
	EBREntityState EntityState = EBREntityState::Idle;

	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Backrooms|AI")
	TObjectPtr<AActor> TargetActor;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|Animation") TObjectPtr<UAnimSequence> IdleAnimation;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|Animation") TObjectPtr<UAnimSequence> WalkAnimation;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|Animation") TObjectPtr<UAnimSequence> RunAnimation;
    UPROPERTY(EditDefaultsOnly, Category="Backrooms|Animation") TObjectPtr<UAnimSequence> AttackAnimation;
    UFUNCTION(NetMulticast, Reliable) void MulticastPlayAttack(APawn* Victim, uint8 Variation);
    int32 AttackCueCount = 0;
    UPROPERTY(Transient) TObjectPtr<UAnimSequence> ActiveAnimation;
    double AttackAnimationEnds=0;
    UPROPERTY(Transient) TObjectPtr<UAudioComponent> ChaseAudio;
    FVector LastAudioLocation=FVector::ZeroVector;
    float AudioStepDistance=0;
    bool bAlternateStep=false;
    bool bAudioWasChasing=false;
    void UpdateGameplayAudio();

	UFUNCTION()
	void OnRep_EntityState();

	UFUNCTION(BlueprintImplementableEvent, Category = "Backrooms|AI")
	void OnEntityStateChanged(EBREntityState NewState);
};
