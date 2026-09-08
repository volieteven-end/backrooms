#include "Player/BRStaminaComponent.h"

#include "Player/BRPlayerCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

UBRStaminaComponent::UBRStaminaComponent()
{
    SetIsReplicatedByDefault(true);
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.05f;
}

void UBRStaminaComponent::BeginPlay()
{
    Super::BeginPlay();
    if (GetOwner()->HasAuthority()) Stamina = GetMaxStamina();
}

void UBRStaminaComponent::SetChasedBy(AActor* Entity, bool bChasing)
{
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Entity || Entity == GetOwner()) return;
    if (bChasing && IsValid(Entity) && !Entity->IsActorBeingDestroyed()) Chasers.Add(Entity);
    else Chasers.Remove(Entity);
    RefreshChasers();
}

void UBRStaminaComponent::RefreshChasers()
{
    for (auto It = Chasers.CreateIterator(); It; ++It)
        if (!It->IsValid() || It->Get()->IsActorBeingDestroyed()) It.RemoveCurrent();

    const bool bChasedNow = !Chasers.IsEmpty();
    if (bBeingChased != bChasedNow)
    {
        bBeingChased = bChasedNow;
        RecoveryRemaining = FMath::Max(0.f, RecoveryDelay);
        OnRep_Stamina();
        GetOwner()->ForceNetUpdate();
    }
}

void UBRStaminaComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* Function)
{
    Super::TickComponent(DeltaTime, TickType, Function);
    auto* Player = Cast<ABRPlayerCharacter>(GetOwner());
    if (!Player || !Player->HasAuthority()) return;
    RefreshChasers();

    // Freeze the reserve during a chase, including at zero. The reserve resumes afterward.
    if (bBeingChased) return;
    const float PreviousStamina = Stamina;
    const bool bWasExhausted = bExhausted;
    const bool bRunning = Player->IsSprinting() && !Player->bIsCrouched &&
        Player->GetCharacterMovement()->IsMovingOnGround() && Player->GetVelocity().SizeSquared2D() > 100.f;
    if (bRunning)
    {
        Stamina = FMath::Clamp(Stamina - FMath::Max(0.f, SprintDrainPerSecond) * DeltaTime, 0.f, GetMaxStamina());
        RecoveryRemaining = FMath::Max(0.f, RecoveryDelay);
        if (Stamina <= 0) bExhausted = true;
    }
    else
    {
        const float RecoverTime = FMath::Max(0.f, DeltaTime - RecoveryRemaining);
        RecoveryRemaining = FMath::Max(0.f, RecoveryRemaining - DeltaTime);
        Stamina = FMath::Clamp(Stamina + FMath::Max(0.f, RecoveryPerSecond) * RecoverTime, 0.f, GetMaxStamina());
        if (Stamina >= FMath::Clamp(SprintResumeThreshold, 1.f, GetMaxStamina())) bExhausted = false;
    }
    if (PreviousStamina != Stamina || bWasExhausted != bExhausted) OnRep_Stamina();
}

void UBRStaminaComponent::OnRep_Stamina()
{
    if (auto* Player = Cast<ABRPlayerCharacter>(GetOwner())) Player->RefreshSprintState();
}

void UBRStaminaComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UBRStaminaComponent, Stamina, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UBRStaminaComponent, bExhausted, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UBRStaminaComponent, bBeingChased, COND_OwnerOnly);
}
