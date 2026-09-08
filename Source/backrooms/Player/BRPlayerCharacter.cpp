#include "Player/BRPlayerCharacter.h"
#include "World/BRHideSpot.h"
#include "Player/BRInventoryComponent.h"
#include "Player/BRStaminaComponent.h"
#include "Player/BRFirstPersonArmsComponent.h"

#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Interaction/BRInteractionComponent.h"
#include "Player/BRDownedComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ABRPlayerCharacter::ABRPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
    // Four-player room: keep hidden pawns relevant so every peer receives hide/exit state.
    bAlwaysRelevant = true;
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->MaxStepHeight=45.0f;
    GetCharacterMovement()->bUseFlatBaseForFloorChecks=true;
    GetCharacterMovement()->PerchRadiusThreshold=8.0f;
    GetCharacterMovement()->bAlwaysCheckFloor=true;
	GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(19.0f, 0.0f, 55.24842f));
	FirstPersonCamera->bUsePawnControlRotation = true;
    FirstPersonCamera->SetEnableFirstPersonFieldOfView(true);
    FirstPersonCamera->SetFirstPersonFieldOfView(90);
    FirstPersonCamera->SetEnableFirstPersonScale(true);
    FirstPersonCamera->SetFirstPersonScale(0.1f);
    FirstPersonArms=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArms"));
    FirstPersonArms->SetupAttachment(FirstPersonCamera);
    FirstPersonArms->SetRelativeLocation(FVector(3,0,-67.5));
    FirstPersonArms->SetRelativeRotation(FRotator(0,-90,0).Quaternion());
    FirstPersonArms->SetRelativeScale3D(FVector(0.9f));
    FirstPersonArms->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    FirstPersonArms->SetOnlyOwnerSee(true);
    FirstPersonArms->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    FirstPersonArms->SetCastShadow(false);
    FirstPersonArms->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    static ConstructorHelpers::FObjectFinder<USkeletalMesh> Arms(TEXT("/Game/ReverseAsset/Player/Characters/Arms/ArmsMesh/SkeletalMeshes/ArmsMesh.ArmsMesh"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> ArmsIdle(TEXT("/Game/ReverseAsset/Player/Animations/Arms/A_Player_Arms_Idle.A_Player_Arms_Idle"));
    static ConstructorHelpers::FObjectFinder<UMaterialInterface> ArmsMaterial(TEXT("/Game/ReverseAsset/Player/Materials/M_Player_Arms.M_Player_Arms"));
    FirstPersonArms->SetSkeletalMeshAsset(Arms.Object);ArmsIdleAnimation=ArmsIdle.Object;
    if(ArmsMaterial.Succeeded())FirstPersonArms->SetMaterial(0,ArmsMaterial.Object);
    FirstPersonArms->SetAnimationMode(EAnimationMode::AnimationSingleNode);
    FirstPersonArms->SetAnimation(ArmsIdle.Object);FirstPersonArms->Play(true);
    // Verified cooked ArmsMesh_Skeleton.ItemSocket (no guessed finger midpoint).
    ItemGrip=CreateDefaultSubobject<USceneComponent>(TEXT("ItemGrip"));
    ItemGrip->SetupAttachment(FirstPersonArms,TEXT("RightHand"));
    ItemGrip->SetRelativeLocation(FVector(2.256446f,-11.683668f,3.849611f));
    ItemGrip->SetRelativeRotation(FRotator(-90,1.265879f,-1.2659f));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> Torch(TEXT("/Game/Gameplay/Items/Reference/Meshes/SM_Flashlight/SM_Flashlight/StaticMeshes/SM_Flashlight.SM_Flashlight"));
    FirstPersonFlashlight=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonFlashlight"));
    FirstPersonFlashlight->SetupAttachment(ItemGrip);FirstPersonFlashlight->SetStaticMesh(Torch.Object);
    FirstPersonFlashlight->SetRelativeScale3D(FVector(1,.875f,1));FirstPersonFlashlight->SetRelativeRotation(FRotator(12.395878f,-166.56046f,-30.501467f));FirstPersonFlashlight->SetRelativeLocation(FVector(1.1792569f,.9982159f,4.339308f));
    FirstPersonFlashlight->SetOnlyOwnerSee(true);FirstPersonFlashlight->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    FirstPersonFlashlight->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonFlashlight->SetCastShadow(false);FirstPersonFlashlight->SetVisibility(false);
    WorldFlashlight=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WorldFlashlight"));
    WorldFlashlight->SetupAttachment(GetMesh(),TEXT("RightHand"));WorldFlashlight->SetStaticMesh(Torch.Object);
    WorldFlashlight->SetOwnerNoSee(true);WorldFlashlight->SetCollisionEnabled(ECollisionEnabled::NoCollision);WorldFlashlight->SetVisibility(false);
    FlashlightLight=CreateDefaultSubobject<USpotLightComponent>(TEXT("FlashlightLight"));FlashlightLight->SetupAttachment(FirstPersonCamera);
    FlashlightLight->SetIntensityUnits(ELightUnits::Lumens);FlashlightLight->SetIntensity(FlashlightLumens);
    FlashlightLight->SetAttenuationRadius(2000);FlashlightLight->SetInnerConeAngle(15);FlashlightLight->SetOuterConeAngle(30);
    FlashlightLight->SetVisibility(false);

	InteractionComponent = CreateDefaultSubobject<UBRInteractionComponent>(TEXT("InteractionComponent"));
	DownedComponent = CreateDefaultSubobject<UBRDownedComponent>(TEXT("DownedComponent"));
    InventoryComponent=CreateDefaultSubobject<UBRInventoryComponent>(TEXT("Inventory"));
    StaminaComponent=CreateDefaultSubobject<UBRStaminaComponent>(TEXT("Stamina"));
    ArmsAnimation=CreateDefaultSubobject<UBRFirstPersonArmsComponent>(TEXT("ArmsAnimation"));
    FirstPersonCan=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("FirstPersonCan"));
    FirstPersonCan->SetupAttachment(ItemGrip);
    FirstPersonCan->SetOnlyOwnerSee(true);FirstPersonCan->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
    FirstPersonCan->SetCollisionEnabled(ECollisionEnabled::NoCollision);FirstPersonCan->SetCastShadow(false);FirstPersonCan->SetVisibility(false);
    static ConstructorHelpers::FObjectFinder<UStaticMesh> CanAsset(TEXT("/Game/Gameplay/Items/AlmondWater/SM_AlmondWater.SM_AlmondWater"));
    FirstPersonCan->SetStaticMesh(CanAsset.Object);
    FirstPersonCan->SetRelativeLocation(FVector(0,0,1.5f));FirstPersonCan->SetRelativeRotation(FRotator(0,30.811337f,0));
    FirstPersonCan->SetRelativeScale3D(FVector(1));

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> HazmatMesh(
		TEXT("/Game/ReverseAsset/Player/Characters/Hazmat/Hazmat/SkeletalMeshes/Hazmat.Hazmat"));
	static ConstructorHelpers::FObjectFinder<UAnimSequence> HazmatIdle(
		TEXT("/Game/ReverseAsset/Player/Animations/Body/A_Player_Breathing_Idle.A_Player_Breathing_Idle"));
    IdleAnimation = HazmatIdle.Object;
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Walk(TEXT("/Game/ReverseAsset/Player/Animations/Body/A_Player_Walk.A_Player_Walk"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> Run(TEXT("/Game/ReverseAsset/Player/Animations/Body/A_Player_Fast_Run.A_Player_Fast_Run"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> CrouchIdle(TEXT("/Game/ReverseAsset/Player/Animations/Body/A_Player_Crouch_Idle.A_Player_Crouch_Idle"));
    static ConstructorHelpers::FObjectFinder<UAnimSequence> CrouchWalk(TEXT("/Game/ReverseAsset/Player/Animations/Body/A_Player_Crouched_Walk.A_Player_Crouched_Walk"));
    WalkAnimation = Walk.Object; RunAnimation = Run.Object; CrouchIdleAnimation = CrouchIdle.Object; CrouchWalkAnimation = CrouchWalk.Object;
	if (HazmatMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(HazmatMesh.Object);
		GetMesh()->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
		GetMesh()->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
		GetMesh()->SetOwnerNoSee(true);
		if (HazmatIdle.Succeeded())
		{
			GetMesh()->SetAnimationMode(EAnimationMode::AnimationSingleNode);
			GetMesh()->SetAnimation(HazmatIdle.Object);
			GetMesh()->Play(true);
		}
	}
}

void ABRPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	ApplySprintState();
    FirstPersonArms->UnHideBoneByName(TEXT("LeftShoulder"));
    FirstPersonArms->SetVisibility(IsLocallyControlled());
    if (GetNetMode()!=NM_DedicatedServer && ArmsIdleAnimation) FirstPersonArms->PlayAnimation(ArmsIdleAnimation,true);
	UE_LOG(LogTemp, Display, TEXT("BR_PLAYER_RUNTIME result=READY location=%s"), *GetActorLocation().ToCompactString());

	if (const APlayerController* PC = Cast<APlayerController>(Controller))
	{
		if (ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsystem->AddMappingContext(DefaultMappingContext, 0);
				}
			}
		}
	}
}

void ABRPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* Enhanced = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (MoveAction) Enhanced->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ABRPlayerCharacter::MoveEnhanced);
		if (LookAction) Enhanced->BindAction(LookAction, ETriggerEvent::Triggered, this, &ABRPlayerCharacter::LookEnhanced);
		if (JumpAction)
		{
			Enhanced->BindAction(JumpAction, ETriggerEvent::Started, this, &ACharacter::Jump);
			Enhanced->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (SprintAction)
		{
			Enhanced->BindAction(SprintAction, ETriggerEvent::Started, this, &ABRPlayerCharacter::StartSprint);
			Enhanced->BindAction(SprintAction, ETriggerEvent::Completed, this, &ABRPlayerCharacter::StopSprint);
		}
		if (CrouchAction) Enhanced->BindAction(CrouchAction, ETriggerEvent::Started, this, &ABRPlayerCharacter::ToggleCrouch);
		if (InteractAction) Enhanced->BindAction(InteractAction, ETriggerEvent::Started, this, &ABRPlayerCharacter::TryInteract);
	}

	// These mappings keep the native framework immediately playable before custom Input Actions are assigned.
	if (!MoveAction)
	{
		PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &ABRPlayerCharacter::MoveForwardLegacy);
		PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &ABRPlayerCharacter::MoveRightLegacy);
	}
	if (!LookAction)
	{
		PlayerInputComponent->BindAxis(TEXT("Turn"), this, &APawn::AddControllerYawInput);
		PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &APawn::AddControllerPitchInput);
	}
	if (!JumpAction)
	{
		PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
		PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	}
	if (!SprintAction)
	{
		PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Pressed, this, &ABRPlayerCharacter::StartSprint);
		PlayerInputComponent->BindAction(TEXT("Sprint"), IE_Released, this, &ABRPlayerCharacter::StopSprint);
	}
	if (!CrouchAction) PlayerInputComponent->BindAction(TEXT("Crouch"), IE_Pressed, this, &ABRPlayerCharacter::ToggleCrouch);
	if (!InteractAction) PlayerInputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &ABRPlayerCharacter::TryInteract);
    PlayerInputComponent->BindAction(TEXT("Flashlight"),IE_Pressed,this,&ABRPlayerCharacter::ToggleFlashlight);
    PlayerInputComponent->BindAction(TEXT("ReplaceBattery"),IE_Pressed,this,&ABRPlayerCharacter::ReplaceBattery);
}

void ABRPlayerCharacter::MoveEnhanced(const FInputActionValue& Value)
{
	const FVector2D Movement = Value.Get<FVector2D>();
	MoveForwardLegacy(Movement.Y);
	MoveRightLegacy(Movement.X);
}

void ABRPlayerCharacter::LookEnhanced(const FInputActionValue& Value)
{
	const FVector2D Look = Value.Get<FVector2D>();
	AddControllerYawInput(Look.X);
	AddControllerPitchInput(Look.Y);
}

void ABRPlayerCharacter::MoveForwardLegacy(const float Value)
{
	if (Controller && !DownedComponent->IsDowned() && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
	}
}

void ABRPlayerCharacter::MoveRightLegacy(const float Value)
{
	if (Controller && !DownedComponent->IsDowned() && !FMath::IsNearlyZero(Value))
	{
		const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
		AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
	}
}

void ABRPlayerCharacter::StartSprint()
{
    bSprintRequested = true;
    RefreshSprintState();
    if (!HasAuthority()) ServerSetSprinting(true);
}

void ABRPlayerCharacter::StopSprint()
{
    bSprintRequested = false;
    RefreshSprintState();
	if (!HasAuthority()) ServerSetSprinting(false);
}

void ABRPlayerCharacter::ServerSetSprinting_Implementation(const bool bNewSprinting)
{
    bSprintRequested = bNewSprinting;
    RefreshSprintState();
}

void ABRPlayerCharacter::RefreshSprintState()
{
    if (!HasAuthority() && !IsLocallyControlled()) return;
    const bool bSprintNow = bSprintRequested && StaminaComponent && StaminaComponent->CanSprint() &&
        !DownedComponent->IsDowned() && !CurrentHideSpot && !bIsCrouched;
    if (bIsSprinting != bSprintNow)
    {
        bIsSprinting = bSprintNow;
        ApplySprintState();
        if (HasAuthority()) ForceNetUpdate();
    }
}

void ABRPlayerCharacter::OnRep_IsSprinting()
{
	ApplySprintState();
}

void ABRPlayerCharacter::ApplySprintState()
{
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
}

void ABRPlayerCharacter::ToggleCrouch()
{
	if (DownedComponent->IsDowned()) return;
	if (bIsCrouched) UnCrouch(); else Crouch();
}

void ABRPlayerCharacter::TryInteract()
{
	InteractionComponent->TryInteract();
}

bool ABRPlayerCharacter::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return IsValid(InstigatorPawn) && InstigatorPawn != this && DownedComponent->IsDowned();
}

FText ABRPlayerCharacter::GetInteractionText_Implementation(APawn* InstigatorPawn) const
{
	return CanInteract_Implementation(InstigatorPawn) ? NSLOCTEXT("Backrooms", "RevivePlayer", "救起队友") : FText::GetEmpty();
}

void ABRPlayerCharacter::Interact_Implementation(APawn* InstigatorPawn)
{
	if (HasAuthority() && CanInteract_Implementation(InstigatorPawn))
	{
		DownedComponent->Revive(InstigatorPawn);
	}
}

void ABRPlayerCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABRPlayerCharacter, CurrentHideSpot);
	DOREPLIFETIME(ABRPlayerCharacter, bIsSprinting);
    DOREPLIFETIME(ABRPlayerCharacter,bHasFlashlight);DOREPLIFETIME(ABRPlayerCharacter,bFlashlightOn);
    DOREPLIFETIME_CONDITION(ABRPlayerCharacter,BatteryCharge,COND_OwnerOnly);DOREPLIFETIME_CONDITION(ABRPlayerCharacter,SpareBatteries,COND_OwnerOnly);
}

void ABRPlayerCharacter::SetCurrentHideSpot(ABRHideSpot* Spot)
{
    if (!HasAuthority()) return;
    CurrentHideSpot = Spot; OnRep_CurrentHideSpot(); ForceNetUpdate();
}
void ABRPlayerCharacter::OnRep_CurrentHideSpot()
{
    const bool bHidingNow = IsValid(CurrentHideSpot);
    if (bHidingNow) Tags.AddUnique(TEXT("BR_Hiding")); else Tags.Remove(TEXT("BR_Hiding"));
    SetActorHiddenInGame(bHidingNow); SetActorEnableCollision(!bHidingNow);
    if (bHidingNow || DownedComponent->IsDowned()) GetCharacterMovement()->DisableMovement();
    else GetCharacterMovement()->SetMovementMode(MOVE_Walking);
}
void ABRPlayerCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if (HasAuthority() && IsValid(CurrentHideSpot)) CurrentHideSpot->ReleaseOccupant();
    Super::EndPlay(Reason);
}
void ABRPlayerCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RefreshSprintState();
    TickEquipment(DeltaTime);
    if (GetNetMode() == NM_DedicatedServer) return;
    const float Speed = GetVelocity().Size2D();
    UAnimSequence* Desired = bIsCrouched ? (Speed > 10 ? CrouchWalkAnimation.Get() : CrouchIdleAnimation.Get()) :
        (Speed > 420 ? RunAnimation.Get() : Speed > 10 ? WalkAnimation.Get() : IdleAnimation.Get());
    if (DownedComponent->IsDowned()) Desired = CrouchIdleAnimation;
    if (Desired && ActiveAnimation != Desired)
    { ActiveAnimation = Desired; GetMesh()->PlayAnimation(Desired, true); }
}

void ABRPlayerCharacter::GetActorEyesViewPoint(FVector& Location,FRotator& Rotation) const
{Location=FirstPersonCamera->GetComponentLocation();Rotation=Controller?Controller->GetControlRotation():GetBaseAimRotation();}
bool ABRPlayerCharacter::GrantFlashlight(float Charge)
{
    if(!HasAuthority() || bHasFlashlight || !InventoryComponent->AddItem(EBRInventoryItem::Flashlight))return false;
    bHasFlashlight=true;bFlashlightOn=Charge>0;BatteryCharge=FMath::Clamp(Charge,0.f,100.f);OnRep_Equipment();ForceNetUpdate();ClientPlayArmsAction(1);return true;
}
bool ABRPlayerCharacter::GrantBattery()
{if(!HasAuthority() || !InventoryComponent->AddItem(EBRInventoryItem::Battery))return false;RefreshInventoryEquipment();ClientPlayArmsAction(3);return true;}
bool ABRPlayerCharacter::GrantAlmondWater()
{if(!HasAuthority() || !InventoryComponent->AddItem(EBRInventoryItem::AlmondWater))return false;ClientPlayArmsAction(3);return true;}
void ABRPlayerCharacter::RefreshInventoryEquipment()
{if(!HasAuthority())return;bHasFlashlight=InventoryComponent->Count(EBRInventoryItem::Flashlight)>0;SpareBatteries=InventoryComponent->Count(EBRInventoryItem::Battery);if(!bHasFlashlight || InventoryComponent->GetEquippedItem()!=EBRInventoryItem::Flashlight)bFlashlightOn=false;OnRep_Equipment();ForceNetUpdate();}
void ABRPlayerCharacter::ClientPlayArmsAction_Implementation(uint8 Action){ArmsAnimation->PlayAction(Action);}
void ABRPlayerCharacter::ToggleFlashlight(){ServerToggleFlashlight();}
void ABRPlayerCharacter::ReplaceBattery(){ServerReplaceBattery();}
void ABRPlayerCharacter::ServerToggleFlashlight_Implementation()
{
    if(!bHasFlashlight || BatteryCharge<=0 || CurrentHideSpot || DownedComponent->IsDowned() || GetWorld()->GetTimeSeconds()<NextEquipmentInput)return;
    InventoryComponent->SelectFirst(EBRInventoryItem::Flashlight);
    bFlashlightOn=!bFlashlightOn;ClientPlayArmsAction(2);NextEquipmentInput=GetWorld()->GetTimeSeconds()+0.2;OnRep_Equipment();ForceNetUpdate();
}
void ABRPlayerCharacter::ServerReplaceBattery_Implementation()
{
    if(!bHasFlashlight || SpareBatteries<=0 || BatteryCharge>=99 || CurrentHideSpot || DownedComponent->IsDowned() || GetWorld()->GetTimeSeconds()<NextEquipmentInput)return;
    if(!InventoryComponent->ConsumeBattery())return;SpareBatteries=InventoryComponent->Count(EBRInventoryItem::Battery);BatteryCharge=100;ClientPlayArmsAction(6);NextEquipmentInput=GetWorld()->GetTimeSeconds()+0.2;ForceNetUpdate();
}
void ABRPlayerCharacter::OnRep_Equipment() {TickEquipment(0);}
void ABRPlayerCharacter::CalcCamera(float DeltaTime,FMinimalViewInfo& OutResult)
{
    Super::CalcCamera(DeltaTime,OutResult);
    // First-person primitives are scaled toward the camera by 0.25. The global
    // 5 cm near plane would therefore cut arms at 20 cm in their authored space.
    // Override only this character view, not the editor, menu or other cameras.
    OutResult.PerspectiveNearClipPlane=FMath::Clamp(FirstPersonNearClipDistance,0.1f,5.f);
}

void ABRPlayerCharacter::TickEquipment(float DeltaTime)
{
    const bool Usable=!CurrentHideSpot && !DownedComponent->IsDowned();
    if(HasAuthority())
    {
        if(!Usable && bFlashlightOn){bFlashlightOn=false;ForceNetUpdate();}
        BatteryTickAccumulator+=DeltaTime;
        if(BatteryTickAccumulator>=1)
        {
            if(bFlashlightOn){BatteryCharge=FMath::Max(0.f,BatteryCharge-BatteryDrainPerSecond*BatteryTickAccumulator);if(BatteryCharge<=0)bFlashlightOn=false;ForceNetUpdate();}
            BatteryTickAccumulator=0;
        }
    }
    if(GetNetMode()==NM_DedicatedServer)return;
    ArmsAnimation->Update(DeltaTime,FirstPersonArms);
    const bool Equipped=InventoryComponent->GetEquippedItem()==EBRInventoryItem::Flashlight;
    FirstPersonArms->SetVisibility(IsLocallyControlled() && Usable && ArmsAnimation->WantsVisibleArms());
    FirstPersonCan->SetVisibility(IsLocallyControlled() && Usable && (InventoryComponent->GetEquippedItem()==EBRInventoryItem::AlmondWater || ArmsAnimation->IsDrinking()));
    FirstPersonFlashlight->SetVisibility(bHasFlashlight && Equipped && Usable && IsLocallyControlled() && !ArmsAnimation->IsDrinking());
    WorldFlashlight->SetVisibility(bHasFlashlight && Equipped && Usable);
    FlashlightLight->SetVisibility(bHasFlashlight && bFlashlightOn && Usable);
    FlashlightLight->SetIntensity(FlashlightLumens);
    if(IsLocallyControlled())
    {
        FlashlightLight->SetRelativeLocation(FVector(20,10,-8));FlashlightLight->SetRelativeRotation(FRotator::ZeroRotator);
    }
    else
    {
        FlashlightLight->SetWorldLocation(GetActorLocation()+FVector(0,0,50));
        FlashlightLight->SetWorldRotation(GetBaseAimRotation());
        WorldFlashlight->SetWorldRotation(GetBaseAimRotation()+FRotator(0,-90,0));
    }
}
