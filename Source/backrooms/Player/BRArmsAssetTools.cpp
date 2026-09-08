#include "Player/BRArmsAssetTools.h"
#if WITH_EDITOR
#include "Animation/AnimSequence.h"
#include "Animation/BlendSpace1D.h"
#include "AssetToolsModule.h"
#include "Factories/BlendSpaceFactory1D.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#endif

bool UBRArmsAssetTools::BuildLocomotionBlendSpace()
{
#if WITH_EDITOR
    const FString Folder=TEXT("/Game/Gameplay/Player/Animations/");
    UAnimSequence* Clips[3];
    const TCHAR* Suffixes[]={TEXT("Idle"),TEXT("Walk"),TEXT("Run")};
    for(int32 I=0;I<3;++I)
    {
        Clips[I]=LoadObject<UAnimSequence>(nullptr,*(Folder+TEXT("A_BR_Arms_Relaxed")+Suffixes[I]));
        if(!Clips[I] || (I>0 && Clips[I]->GetSkeleton()!=Clips[0]->GetSkeleton()))return false;
    }
    const FString Name=TEXT("BS_BR_Arms_EmptyLocomotion");
    auto* Blend=LoadObject<UBlendSpace1D>(nullptr,*(Folder+Name));
    if(!Blend)
    {
        auto* Factory=NewObject<UBlendSpaceFactory1D>();Factory->TargetSkeleton=Clips[0]->GetSkeleton();
        Blend=Cast<UBlendSpace1D>(FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get().CreateAsset(Name,Folder.LeftChop(1),UBlendSpace1D::StaticClass(),Factory));
    }
    if(!Blend || Blend->GetSkeleton()!=Clips[0]->GetSkeleton())return false;
    Blend->Modify();
    while(Blend->GetNumberOfBlendSamples()>0)Blend->DeleteSample(0);
    // BlendParameters has no public setter; edit the same reflected property as the editor.
    auto* AxisProperty=FindFProperty<FStructProperty>(UBlendSpace::StaticClass(),TEXT("BlendParameters"));
    if(!AxisProperty)return false;
    auto* Axis=AxisProperty->ContainerPtrToValuePtr<FBlendParameter>(Blend,0);
    Axis->DisplayName=TEXT("Speed");Axis->Min=0;Axis->Max=600;Axis->GridNum=12;Axis->bSnapToGrid=false;
    const float Speeds[]={0,350,600};
    for(int32 I=0;I<3;++I)if(Blend->AddSample(Clips[I],FVector(Speeds[I],0,0))==INDEX_NONE)return false;
    Blend->TargetWeightInterpolationSpeedPerSec=6.f;
    Blend->bTargetWeightInterpolationEaseInOut=true;
    Blend->ResampleData();
    Blend->MarkPackageDirty();
    FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
    return UPackage::SavePackage(Blend->GetOutermost(),Blend,*FPackageName::LongPackageNameToFilename(Folder+Name,FPackageName::GetAssetPackageExtension()),Args);
#else
    return false;
#endif
}
