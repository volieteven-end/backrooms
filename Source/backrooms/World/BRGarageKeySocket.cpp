#include "World/BRGarageKeySocket.h"
#include "World/BRGarageKeyManager.h"
#include "Player/BRPlayerCharacter.h"
#include "Player/BRDownedComponent.h"
#include "Audio/BRGameplayAudioSubsystem.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABRGarageKeySocket::ABRGarageKeySocket()
{
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=false;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    Plate=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Plate"));Plate->SetupAttachment(RootComponent);
    Keyhole=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Keyhole"));Keyhole->SetupAttachment(RootComponent);
    InsertedKey=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("InsertedKey"));InsertedKey->SetupAttachment(RootComponent);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> PlateAsset(TEXT("/Game/ReverseAsset/ParkingGarage/Meshes/Keys/Coop_Open_low__2__Plate_low/StaticMeshes/Coop_Open_low__2__Plate_low.Coop_Open_low__2__Plate_low"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> HoleAsset(TEXT("/Game/ReverseAsset/ParkingGarage/Meshes/Keys/Coop_Open_low__2__Keyhole_low/StaticMeshes/Coop_Open_low__2__Keyhole_low.Coop_Open_low__2__Keyhole_low"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> KeyAsset(TEXT("/Game/ReverseAsset/ParkingGarage/Meshes/Keys/Key/StaticMeshes/Key.Key"));
    Plate->SetStaticMesh(PlateAsset.Object);Plate->SetRelativeScale3D(FVector(0.3075f));
    Keyhole->SetStaticMesh(HoleAsset.Object);Keyhole->SetRelativeScale3D(FVector(0.3f));
    InsertedKey->SetStaticMesh(KeyAsset.Object);InsertedKey->SetRelativeScale3D(FVector(1.41f));
    for(auto* Mesh:{Plate.Get(),Keyhole.Get(),InsertedKey.Get()})
    {Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);Mesh->SetCanEverAffectNavigation(false);}
    // The saved editor scene and a fresh client both start with an empty keyhole.
    InsertedKey->SetVisibility(false);InsertedKey->SetHiddenInGame(true);
    InteractionTarget=CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionTarget"));InteractionTarget->SetupAttachment(RootComponent);
    InteractionTarget->SetRelativeLocation(FVector(0,1.5f,3));InteractionTarget->SetBoxExtent(FVector(16,3,12));
    InteractionTarget->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionTarget->SetCollisionResponseToAllChannels(ECR_Ignore);InteractionTarget->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
}
void ABRGarageKeySocket::BeginPlay(){Super::BeginPlay();ApplyVisualState();}
bool ABRGarageKeySocket::CanInteract_Implementation(APawn* Pawn) const
{
    const auto* Player=Cast<ABRPlayerCharacter>(Pawn);
    return Player && !Player->GetDownedComponent()->IsDowned() && !Player->GetCurrentHideSpot() &&
        KeyManager && KeyManager->CanInsertKey(this) && !bInserted;
}
FText ABRGarageKeySocket::GetInteractionText_Implementation(APawn*) const
{
    if(bInserted)return FText::Format(NSLOCTEXT("Backrooms","KeySocketInserted","插槽 {0}：钥匙已插入"),SocketNumber);
    if(KeyManager && KeyManager->GetCollectedKeys()>=KeyManager->KeysRequired)
        return FText::Format(NSLOCTEXT("Backrooms","KeySocketInsert","按E插入钥匙 · 插槽 {0}"),SocketNumber);
    return FText::Format(NSLOCTEXT("Backrooms","KeySocketCollectFirst","需要集齐4把钥匙（{0}/4）"),KeyManager?KeyManager->GetCollectedKeys():0);
}
void ABRGarageKeySocket::Interact_Implementation(APawn* Pawn)
{
    if(HasAuthority() && CanInteract_Implementation(Pawn) && KeyManager->TryInsertKey(this))
        if(auto* Player=Cast<ABRPlayerCharacter>(Pawn))Player->ClientPlayArmsAction(3);
}
void ABRGarageKeySocket::MarkInserted()
{
    if(!HasAuthority() || bInserted)return;
    bInserted=true;ApplyVisualState();ForceNetUpdate();
}
bool ABRGarageKeySocket::IsInsertedKeyVisible() const{return InsertedKey && InsertedKey->IsVisible() && !InsertedKey->bHiddenInGame;}
void ABRGarageKeySocket::OnRep_Inserted(){ApplyVisualState();}
void ABRGarageKeySocket::ApplyVisualState()
{
    InsertedKey->SetVisibility(bInserted);InsertedKey->SetHiddenInGame(!bInserted);
    if(bInserted && !bObservedInserted && GetWorld())
        if(auto* Audio=GetWorld()->GetSubsystem<UBRGameplayAudioSubsystem>())Audio->PlayEvent(EBRGameplaySound::KeyInsert,GetActorLocation());
    bObservedInserted=bInserted;
}
void ABRGarageKeySocket::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ABRGarageKeySocket,bInserted);}
