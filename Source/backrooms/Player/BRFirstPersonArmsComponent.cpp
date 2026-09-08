#include "Player/BRFirstPersonArmsComponent.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRInventoryComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "UObject/ConstructorHelpers.h"
UBRFirstPersonArmsComponent::UBRFirstPersonArmsComponent()
{
#define BR_ARM(F,N) {static ConstructorHelpers::FObjectFinder<UAnimSequence> A(TEXT("/Game/ReverseAsset/Player/Animations/Arms/" N "." N));F=A.Object;}
    BR_ARM(Idle,"A_Player_Arms_Idle");BR_ARM(Walk,"A_BR_Arms_Walk");BR_ARM(Run,"A_BR_Arms_Run");
    BR_ARM(FlashlightEquip,"A_Arms_Flashlight_Equip");BR_ARM(FlashlightHold,"A_Arms_Flashlight_Hold");BR_ARM(FlashlightToggle,"A_Arms_Flashlight_Toggle");
    BR_ARM(Grab,"A_Arms_Grab");BR_ARM(Stow,"A_Arms_Stow");BR_ARM(CanEquip,"A_Arms_Can_Equip");BR_ARM(CanHold,"A_Arms_Can_Hold");BR_ARM(Drink,"A_Arms_Drink");
#undef BR_ARM
}
void UBRFirstPersonArmsComponent::PlayAction(uint8 A){PendingAction=A;}
void UBRFirstPersonArmsComponent::Update(float DT,USkeletalMeshComponent* Mesh)
{
    auto* P=Cast<ABRPlayerCharacter>(GetOwner());if(!P || !P->IsLocallyControlled() || !Mesh || !Mesh->GetSkeletalMeshAsset())return;
    const float Speed=P->GetVelocity().Size2D();const bool Moving=Speed>10 && P->GetCharacterMovement()->IsMovingOnGround();
    const auto Item=P->GetInventoryComponent()->GetEquippedItem();
    const bool Held=Item==EBRInventoryItem::Flashlight || Item==EBRInventoryItem::AlmondWater;
    UAnimSequence* Desired=Item==EBRInventoryItem::Flashlight?FlashlightHold.Get():Item==EBRInventoryItem::AlmondWater?CanHold.Get():Moving?(Speed>420?Run.Get():Walk.Get()):Idle.Get();
    FName NewState=Item==EBRInventoryItem::Flashlight?TEXT("FlashlightHold"):Item==EBRInventoryItem::AlmondWater?TEXT("CanHold"):Moving?(Speed>420?TEXT("Run"):TEXT("Walk")):TEXT("Idle");
    if(ActiveAction && Mesh->GetSingleNodeInstance() && Active && Mesh->GetSingleNodeInstance()->GetCurrentTime()>=Active->GetPlayLength()-0.04f)ActiveAction=0;
    if(PendingAction)
    {
        ActiveAction=PendingAction;PendingAction=0;
        UAnimSequence* Clip=ActiveAction==1?FlashlightEquip.Get():ActiveAction==2?FlashlightToggle.Get():ActiveAction==3?Grab.Get():ActiveAction==4?CanEquip.Get():ActiveAction==5?Drink.Get():Stow.Get();
        if(Clip && Clip->GetSkeleton()==Mesh->GetSkeletalMeshAsset()->GetSkeleton()){Active=Clip;Mesh->PlayAnimation(Clip,false);}else ActiveAction=0;
    }
    if(ActiveAction){Desired=Active;NewState=FName(*FString::Printf(TEXT("Action_%d"),ActiveAction));}
    if(Desired && Desired!=Active && Desired->GetSkeleton()==Mesh->GetSkeletalMeshAsset()->GetSkeleton())
    {Active=Desired;Mesh->PlayAnimation(Desired,ActiveAction==0);}
    if(!ActiveAction && !Held && P->bIsCrouched)NewState=Moving?TEXT("CrouchWalk"):TEXT("CrouchIdle");
    State=NewState;
    // DT_Items specifies Both for the can, Right for the flashlight.
    const bool Relaxed = !Held && !ActiveAction;
    const bool Both=Relaxed || Item==EBRInventoryItem::AlmondWater || ActiveAction==5;
    if(Both && Mesh->IsBoneHiddenByName(TEXT("LeftShoulder")))Mesh->UnHideBoneByName(TEXT("LeftShoulder"));
    if(!Both && !Mesh->IsBoneHiddenByName(TEXT("LeftShoulder")))Mesh->HideBoneByName(TEXT("LeftShoulder"),EPhysBodyOp::PBO_None);
    const FRotator View=P->GetBaseAimRotation();if(!bInitialized){PreviousView=View;bInitialized=true;}
    const FRotator Turn=(View-PreviousView).GetNormalized();PreviousView=View;
    Motion=FMath::FInterpTo(Motion,Moving?FMath::Clamp(Speed/350.f,0.f,1.7f):0.f,DT,8.f);
    Phase+=DT*(Speed>420?12.f:8.f);
    const float Breath=FMath::Sin(GetWorld()->GetTimeSeconds()*1.8f)*0.20f;
    const FVector Bob(0,FMath::Sin(Phase)*0.6f*Motion,Breath+FMath::Cos(Phase*2)*0.45f*Motion);
    FVector Base=(Held || ActiveAction)?EquippedOffset:EmptyOffset;
    // Empty arms stay with the torso as the view lowers. Cap the compensation at
    // 55 degrees so looking straight down keeps the mesh's open shoulders offscreen.
    // Equipped/one-shot actions retain their authored camera-relative pose.
    const float BodyPitch = FMath::Max(FRotator::NormalizeAxis(View.Pitch), -55.f);
    const FQuat BodyView = Relaxed ? FRotator(-BodyPitch,0,0).Quaternion() : FQuat::Identity;
    const FVector Target=BodyView.RotateVector(Base+Bob*SwayStrength+FVector(0,0,P->bIsCrouched?-2.f:0.f));
    Mesh->SetRelativeLocation(FMath::VInterpTo(Mesh->GetRelativeLocation(),Target,DT,10.f));
    const FRotator Sway(-FMath::Clamp(Turn.Pitch,-3.f,3.f)*SwayStrength,0,-FMath::Clamp(Turn.Yaw,-3.f,3.f)*SwayStrength);
    const FQuat TargetRotation=BodyView*Sway.Quaternion()*FRotator(0,-90,0).Quaternion();
    Mesh->SetRelativeRotation(FQuat::Slerp(Mesh->GetRelativeRotation().Quaternion(),TargetRotation,FMath::Clamp(DT*12,0.f,1.f)));
    if(!ActiveAction && Moving && !Held)Mesh->SetPlayRate(FMath::Clamp(Speed/(Speed>420?600.f:350.f),0.6f,1.3f));else Mesh->SetPlayRate(1);
}

bool UBRFirstPersonArmsComponent::WantsVisibleArms() const
{return Cast<ABRPlayerCharacter>(GetOwner()) != nullptr;}
