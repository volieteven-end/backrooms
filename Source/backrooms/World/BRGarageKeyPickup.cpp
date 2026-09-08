#include "World/BRGarageKeyPickup.h"
#include "Player/BRPlayerCharacter.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Engine/World.h"

#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "EngineUtils.h"
#include "Net/UnrealNetwork.h"
#include "World/BRGarageKeyManager.h"

ABRGarageKeyPickup::ABRGarageKeyPickup()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
    bAlwaysRelevant = true;
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("KeyMesh"));
	Mesh->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	SetRootComponent(Mesh);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> KeyAsset(TEXT("/Game/ReverseAsset/ParkingGarage/Meshes/Keys/Key/StaticMeshes/Key.Key"));
    Mesh->SetStaticMesh(KeyAsset.Object);Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    PickupTarget=CreateDefaultSubobject<USphereComponent>(TEXT("PickupTarget"));PickupTarget->SetupAttachment(Mesh);
    PickupTarget->SetSphereRadius(5);PickupTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    PickupTarget->SetCollisionResponseToAllChannels(ECR_Ignore);PickupTarget->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
}

bool ABRGarageKeyPickup::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return IsValid(InstigatorPawn) && IsPickupActive();
}

FText ABRGarageKeyPickup::GetInteractionText_Implementation(APawn* InstigatorPawn) const
{
	return CanInteract_Implementation(InstigatorPawn)
		? FText::Format(NSLOCTEXT("Backrooms", "PickupPrompt", "按E拾取：\"{0}\""), NSLOCTEXT("Backrooms", "GarageKeyName", "任务钥匙"))
		: FText::GetEmpty();
}

void ABRGarageKeyPickup::Interact_Implementation(APawn* InstigatorPawn)
{
	if (!HasAuthority() || !CanInteract_Implementation(InstigatorPawn))
	{
		return;
	}

	if(auto* P=Cast<ABRPlayerCharacter>(InstigatorPawn))P->ClientPlayArmsAction(3);
	bCollected = true;
	ApplyPickupState();
	for (TActorIterator<ABRGarageKeyManager> It(GetWorld()); It; ++It)
	{
		It->HandleKeyCollected(this, InstigatorPawn);
		break;
	}
	ForceNetUpdate();
}

void ABRGarageKeyPickup::SetPickupActive(const bool bNewActive)
{
	if (!HasAuthority())
	{
		return;
	}
	bPickupActive = bNewActive;
	bCollected = false;
	ApplyPickupState();
	ForceNetUpdate();
}

void ABRGarageKeyPickup::OnRep_PickupState()
{
	ApplyPickupState();
}

void ABRGarageKeyPickup::ApplyPickupState()
{
    if (bCollected && !bAudioObservedCollected)
        if (auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>()) Audio->PlayEvent(EBRGameplaySound::KeyPickup,GetActorLocation());
    bAudioObservedCollected=bCollected;
	const bool bVisibleAndUsable = IsPickupActive();
	SetActorHiddenInGame(!bVisibleAndUsable);
	SetActorEnableCollision(bVisibleAndUsable);
}

void ABRGarageKeyPickup::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABRGarageKeyPickup, bPickupActive);
	DOREPLIFETIME(ABRGarageKeyPickup, bCollected);
    DOREPLIFETIME(ABRGarageKeyPickup,bContainerLocked);
}
void ABRGarageKeyPickup::SetContainerLocked(bool Locked)
{if(HasAuthority()){bContainerLocked=Locked;ApplyPickupState();ForceNetUpdate();}}
