#include "Tests/BRStaminaSmokeProbe.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRPlayerState.h"
#include "Player/BRStaminaComponent.h"
#include "AI/BREntityCharacter.h"
#include "AIController.h"
#include "Online/BRRoomDirectorySubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

ABRStaminaSmokeProbe::ABRStaminaSmokeProbe()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
    bAlwaysRelevant = true;
}
void ABRStaminaSmokeProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABRStaminaSmokeProbe, Stage);
    DOREPLIFETIME(ABRStaminaSmokeProbe, Player);
}
void ABRStaminaSmokeProbe::Check(bool bOK, const TCHAR* Name)
{
    ++Checks; if (!bOK) ++Failures;
    UE_LOG(LogTemp, Display, TEXT("BR_STAMINA_TEST case=%s result=%s"), Name, bOK ? TEXT("PASS") : TEXT("FAIL"));
}
void ABRStaminaSmokeProbe::SetStage(int32 Next)
{ Stage = Next; StageStarted = GetWorld()->GetTimeSeconds(); ForceNetUpdate(); }
void ABRStaminaSmokeProbe::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
#if !UE_BUILD_SHIPPING
    if (!HasAuthority())
    {
        auto* PC = GetGameInstance()->GetFirstLocalPlayerController();
        auto* Local = PC ? Cast<ABRPlayerCharacter>(PC->GetPawn()) : nullptr;
        if (!Local || !Player) return;
        auto* Stamina = Local->GetStaminaComponent();
        const bool bSubject = Local == Player;
        if (ClientInputStage != Stage)
        {
            ClientInputStage = Stage;
            if (bSubject && Stage >= 1 && Stage <= 4) Local->StartSprint();
            else Local->StopSprint();
        }
        if (bSubject && Stage >= 1 && Stage <= 4) Local->AddMovementInput(FVector::ForwardVector);
        auto Observe = [this,bSubject](bool bOK, FName Name)
        {
            if (bOK && !Observed.Contains(Name))
            { Observed.Add(Name); UE_LOG(LogTemp, Display, TEXT("BR_STAMINA_CLIENT subject=%d case=%s result=PASS"), bSubject, *Name.ToString()); }
        };
        Observe(bSubject && Stage == 1 && Stamina->GetStamina()<98 && Local->GetVelocity().Size2D()>500, TEXT("SPRINT_DRAINS"));
        Observe(bSubject && Stage == 2 && Stamina->IsExhausted() && !Local->IsSprinting(), TEXT("EXHAUSTED"));
        Observe(bSubject && Stage == 3 && Stamina->HasUnlimitedStamina() && Stamina->GetStamina()<1 && Local->GetVelocity().Size2D()>500, TEXT("ZERO_RESERVE_CHASE_SPRINT"));
        Observe(!bSubject && Stage == 3 && !Stamina->HasUnlimitedStamina() && Player->GetStaminaComponent()->GetStamina()==100, TEXT("TARGET_ONLY_AND_PRIVATE_RESERVE"));
        Observe(Stage == 4 && Stamina->HasUnlimitedStamina(), TEXT("TWO_CHASE_TARGETS"));
        Observe(Stage >= 5 && Stage < 99 && !Stamina->HasUnlimitedStamina(), TEXT("CHASE_ENDED"));
        Observe(bSubject && Stage == 6 && Stamina->GetStamina()>20 && !Local->IsSprinting(), TEXT("RECOVERED"));
        return;
    }
    const double Now = GetWorld()->GetTimeSeconds();
    if (!Started) Started = Now;
    if (Stage == 99) return;
    if (Now - Started > 70)
    { Check(false, TEXT("TIMEOUT")); SetStage(99); GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->FinishRound(false); return; }
    if (Stage == 0)
    {
        if (Now - Started < 3) return;
        for (TActorIterator<ABRPlayerCharacter> It(GetWorld()); It; ++It)
            if (auto* PS = It->GetPlayerState<ABRPlayerState>()) { if (PS->IsRoomOwner()) Player=*It; else Other=*It; }
        if (!Player || !Other) return;
        for (TActorIterator<ABREntityCharacter> It(GetWorld()); It; ++It)
        { if (auto* AI = Cast<AAIController>(It->GetController())) { AI->StopMovement(); AI->UnPossess(); } It->SetEntityState(EBREntityState::Returning); Entity=*It; }
        if (!Entity) return;
        auto* Floor = GetWorld()->SpawnActor<AStaticMeshActor>(FVector(-20000,-20000,3950),FRotator::ZeroRotator);
        Floor->GetStaticMeshComponent()->SetMobility(EComponentMobility::Movable);
        Floor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Floor->SetActorScale3D(FVector(400,100,1));
        Player->TeleportTo(FVector(-23000,-20000,4100),FRotator::ZeroRotator,false,true);
        Other->TeleportTo(FVector(-23000,-21000,4100),FRotator::ZeroRotator,false,true);
        SetStage(1); return;
    }
    auto* Stamina = Player->GetStaminaComponent();
    const double Elapsed = Now - StageStarted;
    if (Stage == 1 && Stamina->GetStamina()<=0)
    { Check(Elapsed>=4.5 && Elapsed<12, TEXT("LIMITED_SPRINT_DURATION")); SetStage(2); return; }
    if (Stage == 2 && Elapsed>0.7)
    {
        Check(!Player->IsSprinting() && Player->GetVelocity().Size2D()<400 && Stamina->IsExhausted(), TEXT("SERVER_EXHAUSTION_SLOWS_PLAYER"));
        Entity->SetEntityState(EBREntityState::Chasing,Player);
        Check(Player->IsSprinting() && Stamina->HasUnlimitedStamina(),TEXT("ENTITY_CHASE_RESTARTS_EMPTY_SPRINT"));
        SetStage(3); return;
    }
    if (Stage == 3 && Elapsed>6)
    {
        Check(Player->IsSprinting() && Player->GetVelocity().Size2D()>500 && Stamina->GetStamina()<1, TEXT("CHASE_RUNS_LONGER_THAN_FULL_RESERVE"));
        Check(!Other->GetStaminaComponent()->HasUnlimitedStamina(),TEXT("CHASE_IS_PER_PLAYER"));
        AdditionalEntity=GetWorld()->SpawnActor<AActor>(); Stamina->SetChasedBy(AdditionalEntity,true);
        Entity->SetEntityState(EBREntityState::Chasing,Other); SetStage(4); return;
    }
    if (Stage == 4 && Elapsed>1.5)
    {
        Check(Stamina->HasUnlimitedStamina() && Other->GetStaminaComponent()->HasUnlimitedStamina(),TEXT("MULTIPLE_ENTITIES_AND_RETARGET"));
        AdditionalEntity->Destroy(); Entity->SetEntityState(EBREntityState::Returning); SetStage(5); return;
    }
    if (Stage == 5 && Elapsed>0.8)
    {
        Check(!Stamina->HasUnlimitedStamina() && !Other->GetStaminaComponent()->HasUnlimitedStamina() && !Player->IsSprinting(),TEXT("DESTROY_AND_LOSS_CLEAR_CHASE"));
        SetStage(6); return;
    }
    if (Stage == 6 && Elapsed>4)
    {
        Check(Stamina->GetStamina()>20 && Stamina->CanSprint() && !Player->IsSprinting(),TEXT("NORMAL_RECOVERY_AFTER_CHASE"));
        UE_LOG(LogTemp, Display, TEXT("BR_STAMINA_TEST result=%s checks=%d failures=%d"), Failures ? TEXT("FAIL") : TEXT("PASS"), Checks, Failures);
        SetStage(99); GetGameInstance()->GetSubsystem<UBRRoomDirectorySubsystem>()->FinishRound(Failures==0);
    }
#endif
}
