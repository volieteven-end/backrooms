#include "AI/BREntityCharacter.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Core/BRGameState.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "Player/BRStaminaComponent.h"

#include "AI/BREntityAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "UObject/ConstructorHelpers.h"

ABREntityCharacter::ABREntityCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	SetReplicateMovement(true);
	AIControllerClass = ABREntityAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	GetCharacterMovement()->MaxWalkSpeed = 520.0f;
    ApplyFacingConfiguration();
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Idle.A_SkinStealer_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Walk.A_SkinStealer_Walk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Run.A_SkinStealer_Run"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Attack.A_SkinStealer_Attack"));
    IdleAnimation=Idle.Object; WalkAnimation=Walk.Object; RunAnimation=Run.Object; AttackAnimation=Attack.Object;
}

void ABREntityCharacter::BeginPlay()
{
	Super::BeginPlay();
    // The imported actor is scaled to 0.1875 to fit its mesh. That also shrank
    // its capsule to 6.4 x 16.5 cm, making ordinary door sills frame-dependent.
    // Restore a usable collision body while keeping the rendered feet in place.
    auto* Capsule = GetCapsuleComponent();
    const float ShapeScale = Capsule->GetShapeScale();
    if (ShapeScale > UE_SMALL_NUMBER)
    {
        const float OldHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
        Capsule->SetCapsuleSize(FMath::Max(Capsule->GetUnscaledCapsuleRadius(), 34.f / ShapeScale),
            FMath::Max(Capsule->GetUnscaledCapsuleHalfHeight(), 72.f / ShapeScale));
        const float Lift = Capsule->GetScaledCapsuleHalfHeight() - OldHalfHeight;
        if (Lift > UE_SMALL_NUMBER)
        {
            const FVector WorldLift = GetActorUpVector() * Lift;
            GetMesh()->AddLocalOffset(-GetActorTransform().InverseTransformVector(WorldLift));
            if (HasAuthority()) SetActorLocation(GetActorLocation() + WorldLift, false, nullptr, ETeleportType::TeleportPhysics);
        }
        GetCharacterMovement()->UpdateNavAgent(*Capsule);
    }
    // Existing placed actors can retain serialized component defaults. Enforce the
    // same facing contract on server and clients without changing saved map geometry.
    ApplyFacingConfiguration();
    CacheInitialMeshOffset(GetMesh()->GetRelativeLocation(), GetMesh()->GetRelativeRotation());
    LastAudioLocation=GetActorLocation();
	if (!HasAuthority()) return;
	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	const bool bHasNavData = NavigationSystem
		&& NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) != nullptr;
	UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_NAVIGATION result=%s nav_data=%s"),
		bHasNavData ? TEXT("READY") : TEXT("MISSING"), bHasNavData ? TEXT("present") : TEXT("missing"));
}

void ABREntityCharacter::SetEntityState(const EBREntityState NewState, AActor* NewTarget)
{
	if (HasAuthority())
	{
        auto* PreviousPlayer = EntityState == EBREntityState::Chasing ? Cast<ABRPlayerCharacter>(TargetActor) : nullptr;
        auto* NextPlayer = NewState == EBREntityState::Chasing ? Cast<ABRPlayerCharacter>(NewTarget) : nullptr;
        if (IsValid(PreviousPlayer) && PreviousPlayer != NextPlayer) PreviousPlayer->GetStaminaComponent()->SetChasedBy(this, false);
		EntityState = NewState;
		TargetActor = NewTarget;
        if (IsValid(NextPlayer)) NextPlayer->GetStaminaComponent()->SetChasedBy(this, true);
		OnRep_EntityState();
		ForceNetUpdate();
		UpdateRoar();
	}
}

void ABREntityCharacter::OnRep_EntityState()
{
	OnEntityStateChanged(EntityState);
}

void ABREntityCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABREntityCharacter, EntityState);
	DOREPLIFETIME(ABREntityCharacter, TargetActor);
}

void ABREntityCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (HasAuthority())
    {
        auto* Player = Cast<ABRPlayerCharacter>(TargetActor);
        if (IsValid(Player)) Player->GetStaminaComponent()->SetChasedBy(this, false);
    }
    Super::EndPlay(Reason);
}

float ABREntityCharacter::GetAttackAnimationDuration() const
{
    return AttackAnimation ? AttackAnimation->GetPlayLength() : 1.5f;
}

void ABREntityCharacter::PlayReplicatedAttack(APawn* Victim)
{
    if (!HasAuthority()) return;
    MulticastPlayAttack(Victim, uint8(FMath::RandRange(0, 2)));
}
void ABREntityCharacter::MulticastPlayAttack_Implementation(APawn* Victim, uint8 Variation)
{
    ++AttackCueCount;
    if (GetNetMode()==NM_DedicatedServer) return;
    if (IsValid(RoarAudio)) { RoarAudio->Stop(); RoarAudio->DestroyComponent(); RoarAudio=nullptr; }
    if (auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>()) Audio->PlaySkinStealerAttack(GetActorLocation(), Victim, Variation);
    if (!AttackAnimation) return;
    AttackAnimationEnds=FPlatformTime::Seconds()+AttackAnimation->GetPlayLength();
    ActiveAnimation=AttackAnimation; GetMesh()->PlayAnimation(AttackAnimation,false);
}
void ABREntityCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (HasAuthority()) UpdateRoar();
    if (GetNetMode()!=NM_DedicatedServer) UpdateGameplayAudio();
    if (GetNetMode()==NM_DedicatedServer || FPlatformTime::Seconds()<AttackAnimationEnds || !GetMesh()->GetSkeletalMeshAsset()) return;
    const float Speed=GetVelocity().Size2D();
    UAnimSequence* Desired=Speed>350?RunAnimation.Get():Speed>10?WalkAnimation.Get():IdleAnimation.Get();
    if (Desired && Desired!=ActiveAnimation) { ActiveAnimation=Desired;GetMesh()->PlayAnimation(Desired,true); }
}

void ABREntityCharacter::UpdateGameplayAudio()
{
    auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>();
    if (!Audio) return;
    const bool bChasing=EntityState==EBREntityState::Chasing && !Audio->IsRoundEnding();
    // A live cue can arrive before its state RepNotify. Do not cancel that first
    // cue just because the preceding replicated state was still Patrolling.
    if (!bChasing && (bAudioWasChasing || Audio->IsRoundEnding()) && IsValid(RoarAudio))
    { RoarAudio->Stop(); RoarAudio->DestroyComponent(); RoarAudio=nullptr; }
    bAudioWasChasing=bChasing;
    if (bChasing && !IsValid(ChaseAudio)) ChaseAudio=Audio->PlayEvent(EBRGameplaySound::SkinStealerChase,GetActorLocation(),this);
    if (!bChasing && IsValid(ChaseAudio)) {ChaseAudio->Stop();ChaseAudio->DestroyComponent();ChaseAudio=nullptr;}
    const FVector Current=GetActorLocation();
    const float Distance=FVector::Dist2D(Current,LastAudioLocation);
    LastAudioLocation=Current;
    const float Speed=GetVelocity().Size2D();
    // Distance-based steps follow replicated movement and do not tick while idle or teleporting.
    if (Audio->IsRoundEnding() || Speed<40 || !GetCharacterMovement()->IsMovingOnGround() || Distance>300 || FPlatformTime::Seconds()<AttackAnimationEnds)
    {AudioStepDistance=0;return;}
    AudioStepDistance+=Distance;
    const auto* Settings=GetDefault<UBRGameplayAudioSettings>();
    const float Stride=FMath::Max(50.f,Speed>350?Settings->RunStepDistance:Settings->WalkStepDistance);
    if (AudioStepDistance>=Stride)
    {
        AudioStepDistance=FMath::Fmod(AudioStepDistance,Stride);
        Audio->PlayEvent(bAlternateStep?EBRGameplaySound::SkinStealerStep2:EBRGameplaySound::SkinStealerStep1,Current,nullptr,FMath::FRandRange(0.96f,1.04f));
        bAlternateStep=!bAlternateStep;
    }
}

void ABREntityCharacter::ApplyFacingConfiguration()
{
    // The imported SkinStealer's anatomical forward is mesh +Y, UE movement is +X.
    GetMesh()->SetRelativeRotation(FRotator(0, MeshYawOffset, 0));
    bUseControllerRotationYaw = false;
    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    auto* Movement = GetCharacterMovement();
    Movement->bOrientRotationToMovement = true;
    Movement->bUseControllerDesiredRotation = false;
    Movement->RotationRate = FRotator(0, 540, 0);
}

void ABREntityCharacter::UpdateRoar()
{
    if (!HasAuthority() || EntityState != EBREntityState::Chasing) return;
    const auto* Player = Cast<ABRPlayerCharacter>(TargetActor);
    if (!IsValid(Player) || Player->GetDownedComponent()->IsDowned() ||
        Player->GetCurrentHideSpot() || Player->ActorHasTag(TEXT("BR_Hiding"))) return;
    if (const auto* State = GetWorld()->GetGameState<ABRGameState>())
        if (State->GetLevelPhase() != EBRLevelPhase::Exploring && State->GetLevelPhase() != EBRLevelPhase::ExtractionReady) return;
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextRoarTime) return;
    // Do not reset this on perception changes: sight/hearing churn must not spam cues.
    NextRoarTime = Now + FMath::Max(2.f, GetDefault<UBRGameplayAudioSettings>()->RoarInterval);
    MulticastPlayRoar();
}

void ABREntityCharacter::MulticastPlayRoar_Implementation()
{
    ++RoarCueCount;
    UE_LOG(LogTemp, Display, TEXT("BR_ENTITY_ROAR cue=%d net=%d"), RoarCueCount, int32(GetNetMode()));
    if (GetNetMode() == NM_DedicatedServer) return;
    auto* Audio = GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>();
    if (!Audio || Audio->IsRoundEnding() || (IsValid(RoarAudio) && RoarAudio->IsPlaying())) return;
    // Live multicast events do not replay an old discovery roar to a late observer.
    RoarAudio = Audio->PlayEvent(EBRGameplaySound::SkinStealerRoar, GetActorLocation(), this);
}
