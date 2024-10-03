// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/CapsuleComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "CombatComponent.h"
#include "BuffComponent.h"
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
#include "Weapontypes.h"
#include "GameFramework/Pawn.h"
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

    BuffComp = CreateDefaultSubobject<UBuffComponent>("BuffComp");
    BuffComp->SetIsReplicated(true);

    GetCapsuleComponent()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Camera, ECollisionResponse::ECR_Ignore);
    GetMesh()->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Block);
    GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

    TurningInPlace = ETurningInPlace::ETIP_NotTurning;
    NetUpdateFrequency = 66.f;
    MinNetUpdateFrequency = 33.f;

    DissolveTimeline = CreateDefaultSubobject<UTimelineComponent>("DissolveTimelineComponent");

    AttachedGrenade = CreateDefaultSubobject<UStaticMeshComponent>("Attached Grenade");
    AttachedGrenade->SetupAttachment(GetMesh(), FName("GrenadeSocket"));
    AttachedGrenade->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ABlasterCharacter::BeginPlay()
{
    Super::BeginPlay();

    UpdateHUDHealth();
    check(GetMesh());
    check(GetCharacterMovement());
    Tags.Add("BlasterCharacter");

    SetUpInputMappingContext(DefaultMappingContext);

    if (HasAuthority())
    {
        OnTakeAnyDamage.AddDynamic(this, &ThisClass::ReceiveDamage);
    }

    if (AttachedGrenade)
    {
        AttachedGrenade->SetVisibility(false);
    }
}

void ABlasterCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    RotateInPlace(DeltaTime);
    HideCameraIfCharacterClose();
    PollInit();
}

void ABlasterCharacter::PossessedBy(AController* NewController)
{
    Super::PossessedBy(NewController);
    SetUpInputMappingContext(DefaultMappingContext);
}

void ABlasterCharacter::SetUpInputMappingContext(UInputMappingContext* MappingContext)
{
    if (!MappingContext) return;
    if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PlayerController->GetLocalPlayer()))
        {
            Subsystem->ClearAllMappings();
            Subsystem->AddMappingContext(MappingContext, 0);
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
    }
}

void ABlasterCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION(ABlasterCharacter, OverlappingWeapon, COND_OwnerOnly);
    DOREPLIFETIME(ABlasterCharacter, Health);
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
    if (bElimmed) return;
    Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
    UpdateHUDHealth();
    MulticastHitReactMontage(DamageCauser);
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

void ABlasterCharacter::Elim()
{
    if (IsWeaponEquipped())
    {
        CombatComp->EquippedWeapon->Dropped();
    }

    MulticastElim();
    GetWorldTimerManager().SetTimer(ElimTimer, this, &ABlasterCharacter::ElimTimerFinished, ElimDelay);
}

void ABlasterCharacter::MulticastElim_Implementation()
{

    bElimmed = true;

    if (IsControllerValid())
    {
        BlasterPlayerController->SetHUDWeaponAmmo(0);
        BlasterPlayerController->HideHUDWeaponInfo();
        BlasterPlayerController->HideHUDGrenadeInfo();
        SetIsGameplayDisabled(true);
    }

    PlayElimMontage();

    // Start Disolve effect
    if (DissolveMaterialInstance)
    {
        DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
        GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), -0.55f);
        DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
    }
    StartDissolve();

    if (CombatComp)
    {
        CombatComp->FireButtonPressed(false);
    }

    // Disolve character movement
    GetCharacterMovement()->DisableMovement();
    GetCharacterMovement()->StopMovementImmediately();
    SetUpInputMappingContext(ElimmedMappingContext);

    // Diable Collision
    GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Spawn Elim bot
    if (ElimBotEffect && ElimBotSound)
    {
        FVector ElimBotSpawnPoint(GetActorLocation().X, GetActorLocation().Y, GetActorLocation().Z + 200.f);
        ElimBotComponent = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ElimBotEffect, ElimBotSpawnPoint, GetActorRotation());

        UGameplayStatics::SpawnSoundAtLocation(this, ElimBotSound, GetActorLocation());
    }

    if (IsHideSniperScope())
    {
        ShowSniperScopeWidget(false);
    }
}

bool ABlasterCharacter::IsHideSniperScope()
{
    return IsLocallyControlled() && IsWeaponEquipped() && CombatComp->bAiming &&
           CombatComp->EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle;
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
    ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
    if (BlasterGameMode)
    {
        BlasterGameMode->RequestRespawn(this, Controller);
    }
}

void ABlasterCharacter::Destroyed()
{
    Super::Destroyed();

    if (ElimBotComponent)
    {
        ElimBotComponent->DestroyComponent();
    }

    ABlasterGameMode* BlasterGameMode = Cast<ABlasterGameMode>(UGameplayStatics::GetGameMode(this));
    bool bMatchNotInProgress = BlasterGameMode && BlasterGameMode->GetMatchState() != MatchState::InProgress;

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
    AddControllerYawInput(LookAxisVector.X);
    AddControllerPitchInput(LookAxisVector.Y);
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

void ABlasterCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit);
    // if (bIsCrouched)
    // {
    //     UnCrouch();
    // }
}

void ABlasterCharacter::EquipButtonPressed()
{
    if (HasAuthority() && CombatComp)
    {
        CombatComp->EquipWeapon(OverlappingWeapon);
    }
    else
    {
        ServerEquipButtonPressed();
    }
}

void ABlasterCharacter::ServerEquipButtonPressed_Implementation()
{
    if (!CombatComp) return;
    CombatComp->EquipWeapon(OverlappingWeapon);
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
        }
    }
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

void ABlasterCharacter::HideCameraIfCharacterClose()
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
    if (IsWeaponEquipped() && CombatComp->EquippedWeapon->GetWeaponMesh())
    {
        CombatComp->EquippedWeapon->GetWeaponMesh()->bOwnerNoSee = bIsHidden;
    }
}

void ABlasterCharacter::CalculateAO_Pitch()
{
    AO_Pitch = GetBaseAimRotation().Pitch;
    if (AO_Pitch > 90.f && !IsLocallyControlled())
    {
        // map pitch from [270, 360) to [-90, 0)
        FVector2D InRange(270.f, 360.f);
        FVector2D OutRange(-90.f, 0.f);
        AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
    }
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
        ABlasterGameMode* BlasterGameMode = GetWorld()->GetAuthGameMode<ABlasterGameMode>();
        if (BlasterGameMode)
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

ECombatState ABlasterCharacter::GetCombatState() const
{
    if (!CombatComp) return ECombatState::ECS_MAX;
    return CombatComp->CombatState;
}

bool ABlasterCharacter::IsControllerValid()
{
    return BlasterUtils::CastOrUseExistsActor<ABlasterPlayerController>(BlasterPlayerController, GetController());
}