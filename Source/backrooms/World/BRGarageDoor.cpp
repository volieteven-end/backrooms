#include "World/BRGarageDoor.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

ABRGarageDoor::ABRGarageDoor()
{
    PrimaryActorTick.bCanEverTick=true; bReplicates=true; bAlwaysRelevant=true;
    InteractionBox=CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetBoxExtent(FVector(90,55,130));
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetRootComponent(InteractionBox);
}
void ABRGarageDoor::BeginPlay()
{
    Super::BeginPlay();
    // The visible leaf is the target, including after it swings away from the doorway.
    InteractionBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    CachePanels(); OpenAlpha=0.0f; ApplyDoorState();
}
bool ABRGarageDoor::OwnsPanel(const AActor* Panel) const
{ return Panel && Panel->GetLevel()==GetLevel() && !ControlledActorTag.IsNone() && Panel->ActorHasTag(ControlledActorTag); }
void ABRGarageDoor::CachePanels()
{
    if (Panels.Num() || ControlledActorTag.IsNone()) return;
    TArray<AActor*> Found; UGameplayStatics::GetAllActorsWithTag(this,ControlledActorTag,Found);
    for (AActor* Panel:Found) if (IsValid(Panel) && OwnsPanel(Panel))
    {
        if (auto* Mesh=Panel->FindComponentByClass<UStaticMeshComponent>())
        { Mesh->SetMobility(EComponentMobility::Movable); Mesh->SetCanEverAffectNavigation(false); }
        Panels.Add(Panel); ClosedTransforms.Add(Panel->GetActorTransform());
        Panel->SetActorHiddenInGame(false); Panel->SetActorEnableCollision(true);
    }
}
bool ABRGarageDoor::CanInteract_Implementation(APawn* Pawn) const
{ return IsValid(Pawn) && bUnlockable; }
FText ABRGarageDoor::GetInteractionText_Implementation(APawn*) const
{
    return !bUnlockable?NSLOCTEXT("Backrooms","DoorLocked","门已锁住"):
        bOpen?NSLOCTEXT("Backrooms","DoorClose","关门"):NSLOCTEXT("Backrooms","DoorOpen","开门");
}
void ABRGarageDoor::Interact_Implementation(APawn* Pawn)
{ if (CanInteract_Implementation(Pawn)) SetDoorOpen(!bOpen,Pawn); }
bool ABRGarageDoor::IsClosingBlocked() const
{
    for (int32 I=0;I<Panels.Num();++I) if (Panels[I].IsValid())
    {
        auto* Mesh=Panels[I]->FindComponentByClass<UStaticMeshComponent>();
        if (!Mesh || !Mesh->GetStaticMesh()) continue;
        const FBox Box=Mesh->GetStaticMesh()->GetBoundingBox().TransformBy(ClosedTransforms[I]);
        for (TActorIterator<ACharacter> P(GetWorld());P;++P)
            if (!P->IsHidden() && P->GetActorEnableCollision())
            {
                const auto* C=P->GetCapsuleComponent();
                if (Box.ExpandBy(FVector(C->GetScaledCapsuleRadius()+2,C->GetScaledCapsuleRadius()+2,C->GetScaledCapsuleHalfHeight())).IsInsideOrOn(P->GetActorLocation())) return true;
            }
    }
    return false;
}
bool ABRGarageDoor::SetDoorOpen(bool NewOpen, APawn* Pawn)
{
    if (!HasAuthority() || !bUnlockable || NewOpen==bOpen || GetWorld()->GetTimeSeconds()<NextInteractionTime) return false;
    CachePanels();
    if (!NewOpen && IsClosingBlocked()) return false;
    if (NewOpen && Pawn && bOpenAwayFromPlayer)
        OpenDirection=GetActorTransform().InverseTransformPosition(Pawn->GetActorLocation()).X>=0?1.f:-1.f;
    bOpen=NewOpen; NextInteractionTime=GetWorld()->GetTimeSeconds()+0.18;
    ApplyDoorState(); ForceNetUpdate(); return true;
}
void ABRGarageDoor::SetUnlockable(bool NewUnlockable)
{
    if (!HasAuthority()) return;
    bUnlockable=NewUnlockable; bOpen=false; ApplyDoorState(); ForceNetUpdate();
}
void ABRGarageDoor::OnRep_DoorState() { ApplyDoorState(); }
void ABRGarageDoor::ApplyDoorState()
{
    if (bOpen && !bAudioObservedOpen && GetWorld())
        if (auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>()) Audio->PlayEvent(EBRGameplaySound::DoorOpen,GetActorLocation());
    bAudioObservedOpen=bOpen;
}
void ABRGarageDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); CachePanels();
    if (HasAuthority() && !bOpen && OpenAlpha>0 && IsClosingBlocked()) {bOpen=true;ApplyDoorState();ForceNetUpdate();}
    const float Previous=OpenAlpha;
    OpenAlpha=FMath::FInterpConstantTo(OpenAlpha,bOpen?1.f:0.f,DeltaSeconds,1.f/FMath::Max(0.15f,OpenDuration));
    if (OpenAlpha==Previous) return;
    const float Smooth=OpenAlpha*OpenAlpha*(3-2*OpenAlpha);
    for (int32 I=0;I<Panels.Num();++I) if (Panels[I].IsValid())
    {
        FTransform Pose=ClosedTransforms[I];
        if (bSliding) Pose.AddToTranslation(Pose.TransformVector(SlideOffset)*Smooth);
        else
        {
            float Angle=OpenAngle*OpenDirection;
            if (bDoubleLeaf)
                if (auto* Mesh=Panels[I]->FindComponentByClass<UStaticMeshComponent>())
                    if (Mesh->GetStaticMesh()) Angle=Mesh->GetStaticMesh()->GetBoundingBox().Min.Y < -10?OpenAngle:-OpenAngle;
            // The imported leaves have their origin at the hinge, not at the centre.
            Pose.SetRotation(FQuat(FVector::UpVector,FMath::DegreesToRadians(Angle*Smooth))*Pose.GetRotation());
        }
        Panels[I]->SetActorTransform(Pose,false,nullptr,ETeleportType::TeleportPhysics);
    }
}
void ABRGarageDoor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(ABRGarageDoor,bUnlockable); DOREPLIFETIME(ABRGarageDoor,bOpen); DOREPLIFETIME(ABRGarageDoor,OpenDirection);
}
