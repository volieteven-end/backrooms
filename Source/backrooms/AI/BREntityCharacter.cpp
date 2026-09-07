#include "AI/BREntityCharacter.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"

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
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Idle(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Idle.A_SkinStealer_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Walk.A_SkinStealer_Walk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Run.A_SkinStealer_Run"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Attack(TEXT("/Game/ReverseAsset/ParkingGarage/Characters/SkinStealer/Animations/A_SkinStealer_Attack.A_SkinStealer_Attack"));
    IdleAnimation=Idle.Object; WalkAnimation=Walk.Object; RunAnimation=Run.Object; AttackAnimation=Attack.Object;
}

void ABREntityCharacter::BeginPlay()
{
	Super::BeginPlay();
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
		EntityState = NewState;
		TargetActor = NewTarget;
		OnRep_EntityState();
		ForceNetUpdate();
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
    DOREPLIFETIME(ABREntityCharacter, AttackRevision);
}

void ABREntityCharacter::PlayReplicatedAttack()
{
    if (!HasAuthority()) return;
    ++AttackRevision; OnRep_AttackRevision(); ForceNetUpdate();
}
void ABREntityCharacter::OnRep_AttackRevision()
{
    if (GetNetMode()==NM_DedicatedServer) return;
    if (auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>()) Audio->PlayEvent(EBRGameplaySound::SkinStealerAttack,GetActorLocation());
    if (!AttackAnimation) return;
    AttackAnimationEnds=FPlatformTime::Seconds()+AttackAnimation->GetPlayLength();
    ActiveAnimation=AttackAnimation; GetMesh()->PlayAnimation(AttackAnimation,false);
}
void ABREntityCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
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
