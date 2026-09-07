#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRFirstPersonArmsComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "UObject/UnrealType.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRInventoryContracts,"Backrooms.Inventory.Contracts",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBRInventoryContracts::RunTest(const FString&)
{
    const auto* P=GetDefault<ABRPlayerCharacter>();const auto* Bag=P->GetInventoryComponent();if(!TestNotNull(TEXT("Inventory component"),Bag))return false;
    TestEqual(TEXT("Backpack capacity"),UBRInventoryComponent::BackpackCapacity,9);
    TestEqual(TEXT("Nine backpack plus hand plus two pockets"),Bag->GetSlots().Num(),12);TestEqual(TEXT("Initial SAN 100"),Bag->GetSanity(),100.f);
    TestTrue(TEXT("Inventory replicates"),Bag->GetIsReplicated());
    for(const TCHAR* N:{TEXT("Slots"),TEXT("Sanity"),TEXT("SelectedSlot"),TEXT("EquippedItem")})
    {auto* F=FindFProperty<FProperty>(UBRInventoryComponent::StaticClass(),N);TestTrue(N,F && F->HasAnyPropertyFlags(CPF_Net));}
    TestTrue(TEXT("Low steps remain enabled"),P->GetCharacterMovement()->MaxStepHeight>=35);
    TestTrue(TEXT("Flat floor checks"),bool(P->GetCharacterMovement()->bUseFlatBaseForFloorChecks));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBRArmsMotionAssets,"Backrooms.Inventory.ArmsMotionAssets",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FBRArmsMotionAssets::RunTest(const FString&)
{
    const auto* A=GetDefault<ABRPlayerCharacter>()->GetArmsAnimationComponent();if(!TestNotNull(TEXT("Arms state driver"),A))return false;
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/ReverseAsset/Player/Characters/Arms/ArmsMesh/SkeletalMeshes/ArmsMesh.ArmsMesh"));
    for(UAnimSequence* Clip:{A->Idle.Get(),A->Walk.Get(),A->Run.Get(),A->FlashlightEquip.Get(),A->FlashlightHold.Get(),A->FlashlightToggle.Get(),A->Grab.Get(),A->Stow.Get(),A->CanEquip.Get(),A->CanHold.Get(),A->Drink.Get()})
    {if(TestNotNull(TEXT("Motion asset exists"),Clip)){TestTrue(TEXT("Exact arm skeleton"),Mesh && Clip->GetSkeleton()==Mesh->GetSkeleton());TestTrue(TEXT("Clip is animated"),Clip->GetPlayLength()>0.5f);}}
    return true;
}
#endif
