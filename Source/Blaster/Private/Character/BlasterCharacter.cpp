// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "CombatComponent.h"
#include "BuffComp.h"
#include "LagCompensationComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Animation/AnimInstance.h"
#include "Net/UnrealNetwork.h"
#include "Weapon.h"
#include "Kismet/KismetMathLibrary.h"
#include "BlasterAnimInstance.h"
#include "BlasterPlayerController.h"
#include "TimerManager.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Particles/ParticleSystemComponent.h"
#include "BlasterPlayerState.h"
#include "Blaster.h"
#include "BlasterGameMode.h"
#include "BlasterUtils.h"
#include "CarryItemTypes.h"
#include "GameFramework/Pawn.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "BlasterGameplayStatics.h"
#include "BlasterGameState.h"
#include "BlasterCharacter.h"

ABlasterCharacter::ABlasterCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    SpawnCollisionHandlingMethod = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
    CameraBoom->SetupAttachment(GetMesh());
    CameraBoom->TargetArmLength = 300.0f;
    CameraBoom->bUsePawnControlRotation = true;

    FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
    FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    FollowCamera->bUsePawnControlRotation = false;

    bUseControllerRotationYaw = false;
    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 0.f, 850.f);
    GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
    GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchWalkSpeed;

    OverheadWidget = CreateDefaultSubobject<UWidgetComponent>("OverheadWidget");
    OverheadWidget->SetupAttachment(RootComponent);

    CombatComp = CreateDefaultSubobject<UCombatComponent>("CombatComp");
    CombatComp->SetIsReplicated(true);

    BuffComp = CreateDefaultSubobject<UBuffComp>("BuffComponent");
    BuffComp->SetIsReplicated(true);

    LagCompensationComp = CreateDefaultSubobject<ULagCompensationComponent>("LagCompensationComp");

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    GetMesh()->SetReceivesDecals(false);

    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    NetUpdateFrequency = 66.f;
    MinNetUpdateFrequency = 33.f;

    DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>("DissolveTimelineComponent");
    InvisibilityTimeline = CreateDefaultSubobject<UTimelineComponent>("InvisibilityTimelineComponent");

    AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>("Attached Grenade");
    AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
    AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    SetUpHitShapesSSR();
}

void ABlasterCharacter::SetUpHitShapesSSR()
{
    /**
     * Hit boxes for server - side rewind
     */
    head = CreateDefaultSubobject<UBoxComponent>("head");
    head->SetupAttachment(GetMesh(), "head");
    HitCollisionBoxes.Add("head", head);

    pelvis = CreateDefaultSubobject<UBoxComponent>("pelvis");
    pelvis->SetupAttachment(GetMesh(), "pelvis");
    HitCollisionBoxes.Add("pelvis", pelvis);

    spine_02 = CreateDefaultSubobject<UBoxComponent>("spine_02");
    spine_02->SetupAttachment(GetMesh(), "spine_02");
    HitCollisionBoxes.Add("spine_02", spine_02);

    spine_03 = CreateDefaultSubobject<UBoxComponent>("spine_03");
    spine_03->SetupAttachment(GetMesh(), "spine_03");
    HitCollisionBoxes.Add("spine_03", spine_03);

    upperarm_l = CreateDefaultSubobject<UBoxComponent>("upperarm_l");
    upperarm_l->SetupAttachment(GetMesh(), "upperarm_l");
    HitCollisionBoxes.Add("upperarm_l", upperarm_l);

    upperarm_r = CreateDefaultSubobject<UBoxComponent>("upperarm_r");
    upperarm_r->SetupAttachment(GetMesh(), "upperarm_r");
    HitCollisionBoxes.Add("upperarm_r", upperarm_r);

    lowerarm_l = CreateDefaultSubobject<UBoxComponent>("lowerarm_l");
    lowerarm_l->SetupAttachment(GetMesh(), "lowerarm_l");
    HitCollisionBoxes.Add("lowerarm_l", lowerarm_l);

    lowerarm_r = CreateDefaultSubobject<UBoxComponent>("lowerarm_r");
    lowerarm_r->SetupAttachment(GetMesh(), "lowerarm_r");
    HitCollisionBoxes.Add("lowerarm_r", lowerarm_r);

    hand_l = CreateDefaultSubobject<UBoxComponent>("hand_l");
    hand_l->SetupAttachment(GetMesh(), "hand_l");
    HitCollisionBoxes.Add("hand_l", hand_l);

    hand_r = CreateDefaultSubobject<UBoxComponent>("hand_r");
    hand_r->SetupAttachment(GetMesh(), "hand_r");
    HitCollisionBoxes.Add("hand_r", hand_r);

    backpack = CreateDefaultSubobject<UBoxComponent>("backpack");
    backpack->SetupAttachment(GetMesh(), "backpack");
    HitCollisionBoxes.Add("backpack", backpack);

    blanket_l = CreateDefaultSubobject<UBoxComponent>("blanket_l");
    blanket_l->SetupAttachment(GetMesh(), "blanket_l");
    HitCollisionBoxes.Add("blanket_l", blanket_l);

    blanket_r = CreateDefaultSubobject<UBoxComponent>("blanket_r");
    blanket_r->SetupAttachment(GetMesh(), "blanket_r");
    HitCollisionBoxes.Add("blanket_r", blanket_r);

    thigh_l = CreateDefaultSubobject<UBoxComponent>("thigh_l");
    thigh_l->SetupAttachment(GetMesh(), "thigh_l");
    HitCollisionBoxes.Add("thigh_l", thigh_l);

    thigh_r = CreateDefaultSubobject<UBoxComponent>("thigh_r");
    thigh_r->SetupAttachment(GetMesh(), "thigh_r");
    HitCollisionBoxes.Add("thigh_r", thigh_r);

    calf_l = CreateDefaultSubobject<UBoxComponent>("calf_l");
    calf_l->SetupAttachment(GetMesh(), "calf_l");
    HitCollisionBoxes.Add("calf_l", calf_l);

    calf_r = CreateDefaultSubobject<UBoxComponent>("calf_r");
    calf_r->SetupAttachment(GetMesh(), "calf_r");
    HitCollisionBoxes.Add("calf_r", calf_r);

    foot_l = CreateDefaultSubobject<UBoxComponent>("foot_l");
    foot_l->SetupAttachment(GetMesh(), "foot_l");
    HitCollisionBoxes.Add("foot_l", foot_l);

    foot_r = CreateDefaultSubobject<UBoxComponent>("foot_r");
    foot_r->SetupAttachment(GetMesh(), "foot_r");
    HitCollisionBoxes.Add("foot_r", foot_r);

    for (auto& Box : HitCollisionBoxes)
    {
        if (Box.Value)
        {
            Box.Value->SetCollisionObjectType(ECC_HitBox);
            Box.Value->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
            Box.Value->SetCollisionResponseToChannel(ECC_HitBox, ECollisionResponse::ECR_Block);
            Box.Value->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Box.Value->bReturnMaterialOnMove = true;
        }
    }
}

#if WITH_EDITOR
void ABlasterCharacter::PostEditChangeProperty(FPropertyChangedEvent& Event)
{
    Super::PostEditChangeProperty(Event);
    if (!GetCharacterMovement()) return;

    FName PropertyName = Event.Property ? Event.Property->GetFName() : NAME_None;
    if (PropertyName == GET_MEMBER_NAME_CHECKED(ABlasterCharacter, BaseWalkSpeed))
    {
        GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;
    }
    else if (PropertyName == GET_MEMBER_NAME_CHECKED(ABlasterCharacter, CrouchWalkSpeed))
    {
        GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchWalkSpeed;
    }
}
#endif

void ABlasterCharacter::SetTeamColor(ETeam Team)
{
    if (!GetMesh() || !CharacterMaterialsMap.Contains(Team) || !DissolveMaterialInstanceMap.Contains(Team)) return;
    GetMesh()->SetMaterial(0, CharacterMaterialsMap[Team]);
    DissolveMaterialInstance = DissolveMaterialInstanceMap[Team];
    InitializedMaterial = CharacterMaterialsMap[Team];

    // Set invis material
    if (BuffComp && BuffComp->InvisibilityCharacterMaterialInstanceMap.Contains(Team))
    {
        BuffComp->InvisibilityCharacterMaterialInstance = BuffComp->InvisibilityCharacterMaterialInstanceMap[Team];
    }
}

void ABlasterCharacter::BeginPlay()
{
    Super::BeginPlay();
    SpawnDefaultWeapon();
    UpdateHUDAmmo();

    UpdateHUDHealth();
    UpdateHUDShield();
    check(GetMesh());
    check(GetCharacterMovement());
    Tags.Add("BlasterCharacter");

    SetUpInputMappingContext(InGameMappingContext);

    if (HasAuthority())
    {
        OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
    }

    if (AttachedGrenade)
    {
        AttachedGrenade->SetVisibility(false);
    }

    if (IsControllerValid())
    {
        BlasterPlayerController->OnPlayerCharacterBeginPlay.Broadcast();
    }
}

void ABlasterCharacter::SpawnDefaultWeapon()
{
    if (!HasAuthority()) return;

    if (IsBlasterGameModeValid())
    {
        if (GetWorld() && !bElimmed && DefaultWeaponClass)
        {
            if (AWeapon* StartingWeapon = GetWorld()->SpawnActor<AWeapon>(DefaultWeaponClass))
            {
                StartingWeapon->SetReplicates(true);
                StartingWeapon->bDestroyWeapon = true;
                if (CombatComp)
                {
                    CombatComp->EquipWeapon(StartingWeapon);
                }
            }
        }
    }
}

void ABlasterCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RotateInPlace(DeltaTime);
    HideCharacterIfCameraClose();
    PollInit();
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    SetUpInputMappingContext(InGameMappingContext);
    if (IsControllerValid())
    {
        BlasterPlayerController->OnPlayerCharacterBeginPlay.Broadcast();
    }
}

void ABlasterCharacter::SetUpInputMappingContext(UInputMappingContext* MappingContext)
{
    if (!MappingContext) return;
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            LastMappingContext = MappingContext;
            if (PlayerController->ActorHasTag("BlasterPlayerController") && IsControllerValid())
            {
                if (!BlasterPlayerController->GetLastMappingContext())
                {
                    Subsystem->ClearAllMappings();
                    Subsystem->AddMappingContext(MappingContext, 0);
                }
            }
            else
            {
                Subsystem->ClearAllMappings();
                Subsystem->AddMappingContext(MappingContext, 0);
            }
        }
    }
}

void ABlasterCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
    {
        EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Move);
        EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Look);
        EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started, this, &ThisClass::Jump);
        EnhancedInputComponent->BindAction(EquipAction, ETriggerEvent::Started, this, &ThisClass::EquipButtonPressed);
        EnhancedInputComponent->BindAction(CrouchAction, ETriggerEvent::Started, this, &ThisClass::CrouchButtonPressed);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Started, this, &ThisClass::AimButtonPressed);
        EnhancedInputComponent->BindAction(AimAction, ETriggerEvent::Completed, this, &ThisClass::AimButtonReleased);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Started, this, &ThisClass::FireButtonPressed);
        EnhancedInputComponent->BindAction(FireAction, ETriggerEvent::Completed, this, &ThisClass::FireButtonReleased);
        EnhancedInputComponent->BindAction(ReloadAction, ETriggerEvent::Started, this, &ThisClass::ReloadButtonPressed);
        EnhancedInputComponent->BindAction(ThrowGrenade, ETriggerEvent::Started, this, &ThisClass::ThrowGrenadeButtonPressed);
        EnhancedInputComponent->BindAction(SwapWeapon, ETriggerEvent::Started, this, &ThisClass::SwapWeaponButtonPressed);
    }
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
    DOREPLIFETIME(ABlasterCharacter, Health);
    DOREPLIFETIME(ABlasterCharacter, Shield);
    DOREPLIFETIME(ABlasterCharacter, RightHandRotation);
    DOREPLIFETIME(ABlasterCharacter, bGameplayDisabled);
}

void ABlasterCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    if (CombatComp)
    {
        CombatComp->BlasterCharacter = this;
    }
    if (BuffComp && GetCharacterMovement())
    {
        BuffComp->BlasterCharacter = this;
        BuffComp->SetInitialSpeeds(BaseWalkSpeed, CrouchWalkSpeed, AimWalkSpeed);
        BuffComp->SetInitialJumpVelocity(GetCharacterMovement()->JumpZVelocity);
    }
    if (LagCompensationComp)
    {
        LagCompensationComp->BlasterCharacter = this;
        if (IsControllerValid())
        {
            LagCompensationComp->BlasterPlayerController = BlasterPlayerController;
        }
    }
}

void ABlasterCharacter::MulticastGainedTheLead_Implementation()
{
    if (!CrownSystem) return;
    if (!CrownComponent)
    {
        if (GetCapsuleComponent() && GetMesh())
        {
            CrownComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(CrownSystem,  //
                GetMesh(),                                                              //
                NAME_None,                                                              //
                GetMesh()->GetBoneLocation(FName("head")) + FVector(0.f, 0.f, 40.f),    //
                GetActorRotation(),                                                     //
                EAttachLocation::KeepWorldPosition,                                     //
                false,                                                                  //
                false);
        }
    }
    if (CrownComponent)
    {
        if ((!BuffComp || !BuffComp->IsInvisibilityActive()) && !GetIsElimmed())
        {
            CrownComponent->Activate();
        }
    }
}

void ABlasterCharacter::MulticastLostTheLead_Implementation()
{
    if (CrownComponent)
    {
        CrownComponent->DestroyComponent();
    }
}

void ABlasterCharacter::RotateInPlace(float DeltaTime)
{

    if (IsControllerValid() && GetIsGameplayDisabled())
    {
        bUseControllerRotationYaw = false;
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        return;
    }

    if (GetLocalRole() > ENetRole::ROLE_SimulatedProxy && IsLocallyControlled())
    {
        AimOffset(DeltaTime);
    }
    else
    {
        TimeSinceLastMovementReplication += DeltaTime;
        if (TimeSinceLastMovementReplication > 0.25f)
        {
            OnRep_ReplicatedMovement();
        }
        CalculateAO_Pitch();
    }
}

void ABlasterCharacter::AimOffset(float DeltaTime)
{
    if (!IsWeaponEquipped()) return;
    const float Speed = CalculateSpeed();
    const bool bIsInAir = IsInAir();
    if (Speed == 0.f && !bIsInAir)  // standing still, not jumping
    {
        bRotateRootBone = true;
        const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
        AO_Yaw = DeltaAimRotation.Yaw;
        if (TurningInPlace == ETurningInPlace::ETIP_NotTurning)
        {
            InterpAO_Yaw = AO_Yaw;
        }
        bUseControllerRotationYaw = true;
        TurnInPlace(DeltaTime);
    }
    if (Speed > 0.f || bIsInAir)  // running or jumping
    {
        bRotateRootBone = false;
        StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        AO_Yaw = 0.f;
        bUseControllerRotationYaw = true;
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    }
    CalculateAO_Pitch();
}

void ABlasterCharacter::SimProxiesTurn()
{
    if (!IsWeaponEquipped()) return;
    bRotateRootBone = false;
    const float Speed = CalculateSpeed();
    if (Speed > 0.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_NotTurning;
        return;
    }

    ProxyRotationLastFrame = ProxyRotation;
    ProxyRotation = GetActorRotation();
    ProxyYaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

    if (FMath::Abs(ProxyYaw) > TurnThreshold)
    {
        if (ProxyYaw < -TurnThreshold)
        {
            TurningInPlace = ETurningInPlace::ETIP_Left;
        }
        else
        {
            TurningInPlace = ETurningInPlace::ETIP_Right;
        }
        return;
    }
    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
}

void ABlasterCharacter::PlayFireMontage(bool bAiming)
{
    if (!IsWeaponEquipped()) return;
    FName SectionName = bAiming ? "RifleAim" : "RifleHip";
    PlayMontage(FireWeaponMontage, SectionName);
}

void ABlasterCharacter::PlayHitReactMontage(AActor* DamageCauser)
{
    if (!IsWeaponEquipped() || !DamageCauser) return;
    double Theta = GetDirectionalHitReactAngle(DamageCauser->GetActorLocation());
    FName SectionName = GetDirectionalHitReactSection(Theta);
    PlayMontage(HitReactMontage, SectionName);
}

void ABlasterCharacter::PlayElimMontage()
{
    PlayMontage(ElimMontage);
}

void ABlasterCharacter::PlayThrowGrenadeMontage()
{
    PlayMontage(ThrowGrenadeMontage);
}

void ABlasterCharacter::PlayReloadMontage()
{
    if (!IsWeaponEquipped() || !CombatComp->WeaponTypesToMontageSections.Contains(CombatComp->EquippedWeapon->GetWeaponType())) return;
    FName SectionName = CombatComp->WeaponTypesToMontageSections[CombatComp->EquippedWeapon->GetWeaponType()];
    PlayMontage(ReloadMontage, SectionName);
}

void ABlasterCharacter::PlaySwapWeaponsMontage()
{
    if (!CombatComp || !CombatComp->ShouldSwapWeapons()) return;
    CombatComp->CachePendingSwapWeapons();
    PlayMontage(SwapWeaponsMontage);
}

void ABlasterCharacter::PlayMontage(UAnimMontage* Montage, FName SectionName)
{
    if (!GetMesh()->GetAnimInstance() || !Montage) return;
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    AnimInstance->Montage_Play(Montage);
    if (!SectionName.IsNone())
    {
        AnimInstance->Montage_JumpToSection(SectionName);
    }
}

void ABlasterCharacter::StopAllMontages()
{
    if (!GetMesh()->GetAnimInstance()) return;
    UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
    AnimInstance->StopAllMontages(0.2f);
}

void ABlasterCharacter::ReceiveDamage(
    AActor* DamagedActor, float Damage, const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (bElimmed || !IsBlasterGameModeValid()) return;
    Damage = BlasterGameMode->CalculateDamage(InstigatedBy, GetController(), Damage);

    float DamageToHealth = Damage;
    if (Shield > 0.f)
    {
        if (Shield >= Damage)
        {
            Shield = FMath::Clamp(Shield - Damage, 0.f, MaxShield);
            DamageToHealth = 0.f;
        }
        else
        {
            DamageToHealth = FMath::Clamp(DamageToHealth - Shield, 0.f, Damage);
            Shield = 0.f;
        }
    }

    Health = FMath::Clamp(Health - DamageToHealth, 0.f, MaxHealth);

    UpdateHUDHealth();
    UpdateHUDShield();
    if (CombatComp && CombatComp->CombatState == ECombatState::ECS_Unoccupied)
    {
        MulticastHitReactMontage(DamageCauser);
    }

    if (!GetMesh()->GetAnimInstance()) return;
    CheckIfEliminated(InstigatedBy);
}

void ABlasterCharacter::MulticastHitReactMontage_Implementation(AActor* DamageCauser)
{
    PlayHitReactMontage(DamageCauser);
}

void ABlasterCharacter::OnRep_ReplicatedMovement()
{
    Super::OnRep_ReplicatedMovement();
    SimProxiesTurn();
    TimeSinceLastMovementReplication = 0.f;
}

void ABlasterCharacter::OnRep_RightHandTransform()
{
    if (!IsLocallyControlled() && IsBlasterAnimInstanceValid())
    {
        BlasterAnimInstance->SetRightHandRotation(RightHandRotation);
    }
}

void ABlasterCharacter::ServerUpdateRightHandTransform_Implementation(const FRotator& NewRightHandRotation)
{
    RightHandRotation = NewRightHandRotation;
    if (IsBlasterAnimInstanceValid())
    {
        BlasterAnimInstance->SetRightHandRotation(NewRightHandRotation);
    }
}

void ABlasterCharacter::ServerSelfDestruction_Implementation()
{
    UBlasterGameplayStatics::SelfDestruction(this);
}

void ABlasterCharacter::DropOrDestroyWeapon(AWeapon* Weapon)
{
    if (!Weapon) return;

    if (Weapon->bDestroyWeapon)
    {
        Weapon->Destroy();
    }
    else
    {
        Weapon->Dropped();
    }
}

void ABlasterCharacter::Elim(bool bPlayerLeftGame)
{
    SetCombatState(ECombatState::ECS_Unoccupied);

    if (CombatComp)
    {
        DropOrDestroyWeapon(CombatComp->EquippedWeapon);
        DropOrDestroyWeapon(CombatComp->SecondaryWeapon);
    }
    MulticastElim(bPlayerLeftGame);
}

void ABlasterCharacter::MulticastElim_Implementation(bool bPlayerLeftGame)
{
    bLeftGame = bPlayerLeftGame;
    StopAllMontages();
    bElimmed = true;

    if (IsControllerValid())
    {
        BlasterPlayerController->SetHUDWeaponAmmo(0);
        BlasterPlayerController->HideHUDWeaponInfo();
        BlasterPlayerController->HideHUDGrenadeInfo();
        BlasterPlayerController->ShowHUDCharacterOverlay();
        BlasterPlayerController->SetHUDHealth(0, GetMaxHealth());
        BlasterPlayerController->SetHUDShield(0, GetMaxShield());
        SetIsGameplayDisabled(true);

        if (IsLocallyControlled() && BlasterPlayerController->bPauseWidgetOpen)
        {
            BlasterPlayerController->ShowPauseWidget(this);
        }
    }

    PlayElimMontage();

    // Start Disolve effect
    SetDynamicDissolveMaterialInstance(-0.55f, 200.f);
    StartDissolve();

    if (CombatComp)
    {
        CombatComp->FireButtonPressed(false);
    }

    // Disolve character movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();
    SetUpInputMappingContext(ElimmedMappingContext);

    // Disable Collision
    if (GetCapsuleComponent())
    {
        GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (GetMesh())
    {
        GetMesh()->SetCollisionResponseToChannels(ECollisionResponse::ECR_Ignore);
        GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
    }
    if (AttachedGrenade)
    {
        AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }

    // Spawn Elim bot
    if (ElimBotEffect && ElimBotSound)
    {
        FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
        ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint, GetActorRotation());

        UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
    }

    if (IsAiming())
    {
        CombatComp->SetAiming(false);
    }

    // StopInvis
    if (BuffComp->IsInvisibilityActive())
    {
        GetWorldTimerManager().ClearTimer(BuffComp->InvisibilityBuffTimer);
    }

    if (PickupEffect)
    {
        PickupEffect->DeactivateImmediate();
    }

    if (CrownComponent)
    {
        CrownComponent->DestroyComponent();
    }

    GetWorldTimerManager().SetTimer(ElimTimer, this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::SetDynamicDissolveMaterialInstance(float Dissolve, float Glow)
{
    // Start Disolve effect
    if (DissolveMaterialInstance && GetMesh())
    {
        DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
        GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), Dissolve);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), Glow);
    }
}

double ABlasterCharacter::GetDirectionalHitReactAngle(const FVector& ImpactPoint) const
{
    const FVector Forward = GetActorForwardVector();
    const FVector ImpactLowered(ImpactPoint.X, ImpactPoint.Y, GetActorLocation().Z);
    const FVector ToHit = (ImpactLowered - GetActorLocation()).GetSafeNormal();

    // Forward * ToHIt = |Forward| * |ToHit| * cos(theta)
    const double Dot = FVector::DotProduct(Forward, ToHit);
    double Theta = FMath::Acos(Dot);
    // convert from radians to degrees
    Theta = FMath::RadiansToDegrees(Theta);

    // if CrossProduct points down, Theta should be negative
    const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);

    if (CrossProduct.Z < 0)
    {
        Theta *= -1.f;
    }
    return Theta;
}

FName ABlasterCharacter::GetDirectionalHitReactSection(double Theta) const
{
    FName Section("FromBack");
    if (Theta >= -45.f && Theta < 45.f)
    {
        Section = FName("FromFront");
    }
    else if (Theta >= -135.f && Theta < -45.f)
    {
        Section = FName("FromLeft");
    }
    else if (Theta >= 45.f && Theta < 135.f)
    {
        Section = FName("FromRight");
    }
    return Section;
}

void ABlasterCharacter::ElimTimerFinished()
{
    if (IsBlasterGameModeValid() && !bLeftGame)
    {
        BlasterGameMode->RequestRespawn(this, Controller);
    }
    if (bLeftGame && IsLocallyControlled())
    {
        OnLeftGame.Broadcast();
    }
}

void ABlasterCharacter::ServerLeaveGame_Implementation()
{
    if (!GetWorld()) return;
    BlasterPlayerState = BlasterPlayerState == nullptr ? GetPlayerState<ABlasterPlayerState>() : BlasterPlayerState;
    if (IsBlasterGameModeValid() && BlasterPlayerState)
    {
        BlasterGameMode->PlayerLeftGame(BlasterPlayerState);
    }
}

void ABlasterCharacter::Destroyed()
{
    Super::Destroyed();

    if (ElimBotComponent)
    {
        ElimBotComponent->DestroyComponent();
    }

    bool bMatchNotInProgress = IsBlasterGameModeValid() && BlasterGameMode->GetMatchState() != MatchState::InProgress;
    if (IsWeaponEquipped() && bMatchNotInProgress)
    {
        CombatComp->EquippedWeapon->Destroy();
    }
}

bool ABlasterCharacter::IsInAir()
{
    return GetCharacterMovement()->IsFalling();
}

void ABlasterCharacter::UpdateHUDHealth()
{
    if (IsControllerValid())
    {
        BlasterPlayerController->SetHUDHealth(Health, MaxHealth);
    }
}

void ABlasterCharacter::UpdateHUDShield()
{
    if (IsControllerValid())
    {
        BlasterPlayerController->SetHUDShield(Shield, MaxShield);
    }
}

void ABlasterCharacter::UpdateHUDAmmo()
{
    if (IsControllerValid())
    {
        BlasterPlayerController->SetHUDCarriedAmmo(CombatComp->CarriedAmmo);
        BlasterPlayerController->SetHUDGrenadesAmount(CombatComp->Grenades);
        BlasterPlayerController->ShowHUDGrenadeInfo();
        BlasterPlayerController->ShowHUDWeaponInfo();

        if (IsWeaponEquipped())
        {
            BlasterPlayerController->SetHUDWeaponAmmo(CombatComp->EquippedWeapon->GetAmmo());
            BlasterPlayerController->SetHUDWeaponIcon(CombatComp->EquippedWeapon->WeaponIcon);
        }
    }
}

void ABlasterCharacter::Move(const FInputActionValue& Value)
{
    if (!Controller) return;
    const FVector2D MovementVector = Value.Get<FVector2D>();

    const FRotator ControlRotation = GetControlRotation();
    const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);
    const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightVector = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, MovementVector.X);
    AddMovementInput(RightVector, MovementVector.Y);
}

void ABlasterCharacter::Look(const FInputActionValue& Value)
{
    if (!Controller) return;
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    AddControllerYawInput(LookAxisVector.X * CurrentSensitivity);
    AddControllerPitchInput(LookAxisVector.Y * CurrentSensitivity);
}

void ABlasterCharacter::Jump()
{
    if (bIsCrouched)
    {
        UnCrouch();
    }
    else
    {
        Super::Jump();
    }
}

void ABlasterCharacter::EquipButtonPressed()
{
    if (!OverlappingWeapon || !CombatComp) return;
    CombatComp->bLocallyReloading = false;
    if (CombatComp->CombatState != ECombatState::ECS_SwappingWeapons)
    {
        ServerEquipButtonPressed();
    }
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
    if (!OverlappingWeapon || !CombatComp) return;
    CombatComp->EquipWeapon(OverlappingWeapon);
}

void ABlasterCharacter::SwapWeaponButtonPressed()
{
    if (CombatComp->CombatState == ECombatState::ECS_Unoccupied && !IsLocallyReloading())
    {
        if (CombatComp->ShouldSwapWeapons() && !HasAuthority())
        {
            StopAllMontages();
            PlaySwapWeaponsMontage();
            bFinishSwapping = false;
            CombatComp->CombatState = ECombatState::ECS_SwappingWeapons;
        }
        ServerSwapButtonPressed();
    }
    else
    {
        bFinishSwapping = true;
    }
}

void ABlasterCharacter::ServerSwapButtonPressed_Implementation()
{
    if (!CombatComp || !CombatComp->ShouldSwapWeapons()) return;
    CombatComp->SwapWeapons();
}

void ABlasterCharacter::CrouchButtonPressed()
{
    if (bIsCrouched)
    {
        UnCrouch();
    }
    else
    {
        Crouch();
    }
}

void ABlasterCharacter::AimButtonPressed()
{
    if (!CombatComp) return;
    CombatComp->SetAiming(true);
}

void ABlasterCharacter::AimButtonReleased()
{
    if (!CombatComp) return;
    CombatComp->SetAiming(false);
}

void ABlasterCharacter::ReloadButtonPressed()
{
    if (!CombatComp) return;
    CombatComp->Reload();
}

void ABlasterCharacter::FireButtonPressed()
{
    if (!IsWeaponEquipped()) return;
    CombatComp->FireButtonPressed(true);
}

void ABlasterCharacter::FireButtonReleased()
{
    if (!IsWeaponEquipped()) return;
    CombatComp->FireButtonPressed(false);
}

void ABlasterCharacter::ThrowGrenadeButtonPressed()
{
    if (!CombatComp) return;
    CombatComp->ThrowGrenade();
}

void ABlasterCharacter::PollInit()
{
    if (!BlasterPlayerState)
    {
        BlasterPlayerState = GetPlayerState<ABlasterPlayerState>();
        if (BlasterPlayerState)
        {
            BlasterPlayerState->AddToScore(0.f);
            BlasterPlayerState->AddToDefeats(0);
            BlasterPlayerState->SetKilledBy(FName());
            SetTeamColor(BlasterPlayerState->GetTeam());
            if (IsCharacterGainedTheLead())
            {
                MulticastGainedTheLead();
            }
        }
    }
}

bool ABlasterCharacter::IsCharacterGainedTheLead()
{
    if (ABlasterGameState* BlasterGameState = Cast<ABlasterGameState>(UGameplayStatics::GetGameState(this)))
    {
        if (BlasterGameState->TopScoringPlayers.Contains(BlasterPlayerState))
        {
            return true;
        }
    }
    return false;
}

void ABlasterCharacter::OnRep_OverlappingWeapon(AWeapon* LastWeapon)
{
    if (OverlappingWeapon)
    {
        OverlappingWeapon->ShowPickupWidget(true);
    }
    if (LastWeapon)
    {
        LastWeapon->ShowPickupWidget(false);
    }
}

void ABlasterCharacter::OnRep_Health()
{
    UpdateHUDHealth();
}

void ABlasterCharacter::OnRep_Shield()
{
    UpdateHUDShield();
}

void ABlasterCharacter::HideCharacterIfCameraClose()
{
    if (!IsLocallyControlled()) return;
    if ((FollowCamera->GetComponentLocation() - GetActorLocation()).Size() < CameraThreshold)
    {
        HideCamera(true);
    }
    else
    {
        HideCamera(false);
    }
}

void ABlasterCharacter::HideCamera(bool bIsHidden)
{
    GetMesh()->SetVisibility(!bIsHidden);
    if (IsWeaponEquipped() && CombatComp->EquippedWeapon->GetItemMesh())
    {
        CombatComp->EquippedWeapon->GetItemMesh()->bOwnerNoSee = bIsHidden;
    }
    if (IsSecondaryWeapon() && CombatComp->SecondaryWeapon->GetItemMesh())
    {
        CombatComp->SecondaryWeapon->GetItemMesh()->bOwnerNoSee = bIsHidden;
    }
}

void ABlasterCharacter::CalculateAO_Pitch()
{
    AO_Pitch = GetBaseAimRotation().GetNormalized().Pitch;
}

float ABlasterCharacter::CalculateSpeed()
{
    FVector Velocity = GetVelocity();
    Velocity.Z = 0.f;
    return Velocity.Size();
}

void ABlasterCharacter::CheckIfEliminated(AController* InstigatorController)
{
    if (FMath::IsNearlyZero(Health))
    {
        if (IsBlasterGameModeValid())
        {
            BlasterUtils::CastOrUseExistsActor<ABlasterPlayerController>(BlasterPlayerController, GetController());
            ABlasterPlayerController* AttackerController = Cast<ABlasterPlayerController>(InstigatorController);
            BlasterGameMode->PlayerElimmed(this, BlasterPlayerController, AttackerController);
        }
    }
}

void ABlasterCharacter::UpdateDissolveMaterial(float DissolveValue)
{
    if (DynamicDissolveMaterialInstance)
    {
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
    }
}

void ABlasterCharacter::StartDissolve()
{
    DissolveTrack.BindDynamic(this, &ThisClass::UpdateDissolveMaterial);
    if (DissolveCurve && DissolveTimeline)
    {
        DissolveTimeline->AddInterpFloat(DissolveCurve, DissolveTrack);
        DissolveTimeline->Play();
    }
}

void ABlasterCharacter::TurnInPlace(float DeltaTime)
{
    if (AO_Yaw > 90.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_Right;
    }
    else if (AO_Yaw < -90.f)
    {
        TurningInPlace = ETurningInPlace::ETIP_Left;
    }
    if (TurningInPlace != ETurningInPlace::ETIP_NotTurning)
    {
        InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaTime, 4.f);
        AO_Yaw = InterpAO_Yaw;
        if (FMath::Abs(AO_Yaw) < 15.f)
        {
            TurningInPlace = ETurningInPlace::ETIP_NotTurning;
            StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
        }
    }
}

void ABlasterCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
    if (OverlappingWeapon)
    {
        OverlappingWeapon->ShowPickupWidget(false);
    }
    OverlappingWeapon = Weapon;
    if (!IsLocallyControlled() || !OverlappingWeapon) return;
    OverlappingWeapon->ShowPickupWidget(true);
}

bool ABlasterCharacter::IsWeaponEquipped()
{
    return CombatComp && CombatComp->EquippedWeapon;
}

bool ABlasterCharacter::IsSecondaryWeapon()
{
    return CombatComp && CombatComp->SecondaryWeapon;
}

bool ABlasterCharacter::IsAiming()
{
    return CombatComp && CombatComp->bAiming;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon()
{
    if (!CombatComp) return nullptr;
    return CombatComp->EquippedWeapon;
}

FVector ABlasterCharacter::GetHitTarget() const
{
    if (!CombatComp) return FVector();
    return CombatComp->HitTarget;
}

void ABlasterCharacter::SetHitTarget(const FVector& NewHitTarget)
{
    if (!CombatComp) return;
    CombatComp->HitTarget = NewHitTarget;
}

ECombatState ABlasterCharacter::GetCombatState() const
{
    if (!CombatComp) return ECombatState::ECS_MAX;
    return CombatComp->CombatState;
}

void ABlasterCharacter::SetCombatState(ECombatState NewCombatState)
{
    if (!CombatComp) return;
    CombatComp->CombatState = NewCombatState;
}

AWeapon* ABlasterCharacter::GetEquippedWeapon() const
{
    if (!CombatComp) return nullptr;
    return CombatComp->EquippedWeapon;
}

AWeapon* ABlasterCharacter::GetSecondaryWeapon() const
{
    if (!CombatComp) return nullptr;
    return CombatComp->SecondaryWeapon;
}

void ABlasterCharacter::SetDefaultMaterial()
{
    if (!GetMesh()) return;
    GetMesh()->SetMaterial(0, InitializedMaterial);
}

void ABlasterCharacter::SetMaterial(UMaterialInterface* NewMaterial)
{
    if (!GetMesh()) return;
    GetMesh()->SetMaterial(0, NewMaterial);
}

bool ABlasterCharacter::IsLocallyReloading() const
{
    return CombatComp && CombatComp->bLocallyReloading;
}

bool ABlasterCharacter::IsHoldingTheFlag() const
{
    return CombatComp && CombatComp->bHoldingTheFlag;
}

bool ABlasterCharacter::IsControllerValid()
{
    return BlasterUtils::CastOrUseExistsActor<ABlasterPlayerController>(BlasterPlayerController, GetController());
}

bool ABlasterCharacter::IsBlasterGameModeValid()
{
    if (!GetWorld()) return false;
    BlasterGameMode = BlasterGameMode == nullptr ? GetWorld()->GetAuthGameMode<ABlasterGameMode>() : BlasterGameMode;
    return BlasterGameMode != nullptr;
}

bool ABlasterCharacter::IsBlasterAnimInstanceValid()
{
    if (!GetMesh()->GetAnimInstance()) return false;
    BlasterAnimInstance = BlasterAnimInstance == nullptr ? Cast<UBlasterAnimInstance>(GetMesh()->GetAnimInstance()) : BlasterAnimInstance;
    return BlasterAnimInstance != nullptr;
}