#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/BRPlayerCharacter.h"
#include "World/BRLootCabinet.h"
#include "World/BRSupplyPickup.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UnrealType.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRFirstPersonAssets,"Backrooms.Items.FirstPersonAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBRFirstPersonAssets::RunTest(const FString&)
{
    const auto* P=GetDefault<ABRPlayerCharacter>();USkeletalMeshComponent* Arms=nullptr;
    TArray<USkeletalMeshComponent*> Meshes;P->GetComponents(Meshes);
    for(auto* M:Meshes)if(M->GetFName()==TEXT("FirstPersonArms"))Arms=M;
    if(TestNotNull(TEXT("Separate first-person arms component"),Arms))
    {
        TestTrue(TEXT("Only owning player sees arms"),bool(Arms->bOnlyOwnerSee));
        TestFalse(TEXT("Arms not hidden from owner"),bool(Arms->bOwnerNoSee));
        TestEqual(TEXT("Arms have no collision"),Arms->GetCollisionEnabled(),ECollisionEnabled::NoCollision);
        auto* Seq=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/ReverseAsset/Player/Animations/Arms/A_Player_Arms_Idle.A_Player_Arms_Idle"));
        if(TestNotNull(TEXT("Arms animation"),Seq) && TestNotNull(TEXT("Arms mesh"),Arms->GetSkeletalMeshAsset()))
            TestTrue(TEXT("Exact skeleton match (no mismatched body animation)"),Seq->GetSkeleton()==Arms->GetSkeletalMeshAsset()->GetSkeleton());
    }
    TestFalse(TEXT("Player starts without flashlight"),P->HasFlashlight());
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRItemReplication,"Backrooms.Items.ReplicationContracts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBRItemReplication::RunTest(const FString&)
{
    auto Net=[this](UClass* C,const TCHAR* N){const FProperty* P=FindFProperty<FProperty>(C,N);TestTrue(N,P && P->HasAnyPropertyFlags(CPF_Net));};
    Net(ABRPlayerCharacter::StaticClass(),TEXT("bHasFlashlight"));Net(ABRPlayerCharacter::StaticClass(),TEXT("bFlashlightOn"));
    Net(ABRPlayerCharacter::StaticClass(),TEXT("BatteryCharge"));Net(ABRPlayerCharacter::StaticClass(),TEXT("SpareBatteries"));
    Net(ABRGarageDoor::StaticClass(),TEXT("bOpen"));Net(ABRGarageDoor::StaticClass(),TEXT("OpenDirection"));
    Net(ABRLootCabinet::StaticClass(),TEXT("LootType"));Net(ABRSupplyPickup::StaticClass(),TEXT("bCollected"));
    TestTrue(TEXT("Open duration is nonzero"),GetDefault<ABRGarageDoor>()->OpenDuration>0.1f);
    return true;
}
#endif
