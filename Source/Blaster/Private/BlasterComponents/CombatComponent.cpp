// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/SkeletalMeshSocket.h"
#include "Components/SkeletalMeshComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "BlasterCharacter.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Weapon.h"
#include "BlasterPlayerController.h"
#include "TimerManager.h"
#include "BlasterUtils.h"
#include "Sound/SoundBase.h"
#include "DrawDebugHelpers.h"
#include "CombatComponent.h"

UCombatComponent::UCombatComponent()
{
    PrimaryComponentTick.bCanEverTick = true;

    BaseWalkSpeed = 600.f;
    AimWalkSpeed = 450.f;
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (BlasterCharacter && BlasterCharacter->IsLocallyControlled())
    {
        FHitResult HitResult;
        TraceUnderCrosshairs(HitResult);
        HitTarget = HitResult.ImpactPoint;

        // DrawDebugSphere(GetWorld(), HitResult.ImpactPoint, 15.f, 12, FColor::Red, false, 0.1f);

        SetHUDCrosshairs(DeltaTime);
        InterpFOV(DeltaTime);
    }
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!BlasterCharacter || !BlasterCharacter->GetFollowCamera()) return;
    check(BlasterCharacter->GetCharacterMovement());
    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

    DefaultFOV = BlasterCharacter->GetFollowCamera()->FieldOfView;
    CurrentFOV = DefaultFOV;

    if (BlasterCharacter->HasAuthority())
    {
        InitializeCarriedAmmo();
    }
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCombatComponent, EquippedWeapon);
    DOREPLIFETIME(UCombatComponent, bAiming);
    DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
    DOREPLIFETIME(UCombatComponent, CombatState);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
    if (!BlasterCharacter || !WeaponToEquip) return;
    if (EquippedWeapon)
    {
        EquippedWeapon->Dropped();
    }
    EquippedWeapon = WeaponToEquip;

    HandleEquipWeapon();
    EquippedWeapon->SetOwner(BlasterCharacter);
    EquippedWeapon->SetHUDAmmo();
    SetCarriedAmmo();

    if (IsControllerValid())
    {
        BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
        BlasterController->SetHUDWeaponIcon(EquippedWeapon->WeaponIcon);
        BlasterController->ShowHUDWeaponAmmoBox();
    }
    if (EquippedWeapon->IsEmpty())
    {
        Reload();
    }
}

void UCombatComponent::OnRep_EquippedWeapon()
{
    if (!EquippedWeapon || !BlasterCharacter) return;
    HandleEquipWeapon();
    if (IsControllerValid())
    {
        BlasterController->SetHUDWeaponIcon(EquippedWeapon->WeaponIcon);
        BlasterController->ShowHUDWeaponAmmoBox();
    }
}

void UCombatComponent::HandleEquipWeapon()
{
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equiped);
    const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName("RightHandSocket");
    if (HandSocket)
    {
        HandSocket->AttachActor(EquippedWeapon, BlasterCharacter->GetMesh());
    }

    if (EquippedWeapon->EquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, BlasterCharacter->GetActorLocation());
    }

    BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
    BlasterCharacter->bUseControllerRotationYaw = true;

    if (BlasterUtils::CastOrUseExistsActor<ABlasterPlayerController>(BlasterController, BlasterCharacter->GetController()))
    {
        BlasterController->SetHUDWeaponIcon(EquippedWeapon->WeaponIcon);
    }
}

void UCombatComponent::Reload()
{
    if (CanReload())
    {
        ServerReload();
    }
}

void UCombatComponent::ServerReload_Implementation()
{
    if (!BlasterCharacter || !EquippedWeapon) return;
    CombatState = ECombatState::ECS_Reloading;
    HandleReload();
}

void UCombatComponent::FinishReloading()
{
    if (!BlasterCharacter) return;
    if (BlasterCharacter->HasAuthority())
    {
        CombatState = ECombatState::ECS_Unoccupied;
        UpdateAmmoValues();
    }
    if (bFireButtonPressed)
    {
        Fire();
    }
}

void UCombatComponent::UpdateAmmoValues()
{
    if (!BlasterCharacter || !EquippedWeapon || !HasEquippedWeaponKey()) return;
    int32 ReloadAmount = GetAmountToReload();
    CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= ReloadAmount;
    CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
    if (IsControllerValid())
    {
        BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
    }
    EquippedWeapon->AddAmmo(-ReloadAmount);
}

bool UCombatComponent::CanReload()
{
    return EquippedWeapon &&                                                //
           EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() &&  //
           CarriedAmmo > 0 &&                                               //
           CombatState != ECombatState::ECS_Reloading;                      //
}

void UCombatComponent::OnRep_CombatState()
{
    switch (CombatState)
    {
        case ECombatState::ECS_Reloading: HandleReload(); break;
        case ECombatState::ECS_Unoccupied:
            if (bFireButtonPressed)
            {
                Fire();
            }
            break;
        default: break;
    }
}

void UCombatComponent::HandleReload()
{
    BlasterCharacter->PlayReloadMontage();
}

int32 UCombatComponent::GetAmountToReload()
{
    if (!EquippedWeapon || !HasEquippedWeaponKey()) return 0;
    int32 RoomInMag = EquippedWeapon->GetMagCapacity() - EquippedWeapon->GetAmmo();
    int32 AmmountCarried = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
    int32 Least = FMath::Min(RoomInMag, AmmountCarried);
    return FMath::Clamp(RoomInMag, 0, Least);
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
    bAiming = bIsAiming;
    ServerSetAiming(bIsAiming);
    if (BlasterCharacter)
    {
        BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
    bAiming = bIsAiming;
    if (BlasterCharacter)
    {
        BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
    bFireButtonPressed = bPressed;
    if (bFireButtonPressed)
    {
        Fire();
    }
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
    MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
    if (!BlasterCharacter || !EquippedWeapon || CombatState != ECombatState::ECS_Unoccupied) return;
    BlasterCharacter->PlayFireMontage(bAiming);
    EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
    if (!BlasterCharacter || !GEngine || !GEngine->GameViewport) return;
    FVector2D ViewportSize;
    GEngine->GameViewport->GetViewportSize(ViewportSize);
    FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
    FVector CrosshairWorldPosition;
    FVector CrosshairWorldDirection;
    bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
        UGameplayStatics::GetPlayerController(this, 0), CrosshairLocation, CrosshairWorldPosition, CrosshairWorldDirection);

    if (bScreenToWorld)
    {
        FVector Start = CrosshairWorldPosition;
        float DistanceToCharacter = (BlasterCharacter->GetActorLocation() - Start).Size();
        Start += CrosshairWorldDirection * (DistanceToCharacter + 80.f);
        FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH;
        GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECollisionChannel::ECC_Visibility);
        if (TraceHitResult.GetActor() && TraceHitResult.GetActor()->Implements<UInteractWithCrosshairsInterface>())
        {
            HUDPackage.CrosshairsColor = FLinearColor::Red;
            CrosshairCharacterFactor = 0.7f;
        }
        else
        {
            HUDPackage.CrosshairsColor = FLinearColor::White;
            CrosshairCharacterFactor = 0.f;
        }
        if (!TraceHitResult.bBlockingHit)
        {
            TraceHitResult.ImpactPoint = End;
        }
    }
}

void UCombatComponent::SetHUDCrosshairs(float DeltaTime)
{

    if (BlasterCharacter && IsControllerValid())
    {
        HUD = HUD == nullptr ? Cast<ABlasterHUD>(BlasterController->GetHUD()) : HUD;
        if (!HUD) return;
        if (!BlasterCharacter || BlasterCharacter->GetIsGameplayDisabled() || !EquippedWeapon)
        {
            HUD->SetIsDrawCrosshair(false);
        }
        else
        {
            HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
            HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
            HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
            HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
            HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
            HUDPackage.CrosshairSpread = GetCrosshairsSpread(DeltaTime);
            HUD->SetHUDPackage(HUDPackage);
            HUD->SetIsDrawCrosshair(true);
        }
    }
}

float UCombatComponent::GetCrosshairsSpread(float DeltaTime)
{
    // Calculate crosshair spread
    //[0, 600] -> [0, 1]
    FVector2D WalkSpeedRange(0.f, BlasterCharacter->bIsCrouched ? BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched
                                                                : BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed);

    FVector2D VelocityMultiplayerRange(0.f, BlasterCharacter->bIsCrouched ? 0.5f : 1.f);
    FVector Velocity = BlasterCharacter->GetVelocity();
    Velocity.Z = 0.f;

    CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplayerRange, Velocity.Size());
    if (BlasterCharacter->IsInAir())
    {
        CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 2.25f, DeltaTime, 2.2f);
    }
    else
    {
        CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 0.f, DeltaTime, 30.f);
    }
    if (bAiming)
    {
        CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.58f, DeltaTime, 30.f);
    }
    else
    {
        CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 0.f, DeltaTime, 30.f);
    }

    CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaTime, 40.f);

    return 0.5f +                      //
           CrosshairVelocityFactor +   //
           CrosshairInAirFactor -      //
           CrosshairAimFactor +        //
           CrosshairCharacterFactor +  //
           CrosshairShootingFactor;
}

void UCombatComponent::InterpFOV(float DeltaTime)
{
    if (!EquippedWeapon || !BlasterCharacter || !BlasterCharacter->GetFollowCamera()) return;
    if (bAiming)
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
    }
    else
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
    }
    BlasterCharacter->GetFollowCamera()->SetFieldOfView(CurrentFOV);
}

void UCombatComponent::Fire()
{
    if (CanFire())
    {
        bCanFire = false;
        ServerFire(HitTarget);
        if (!EquippedWeapon) return;
        CrosshairShootingFactor = 0.85f;
        StartFireTimer();
    }
}

void UCombatComponent::StartFireTimer()
{
    if (!EquippedWeapon || !BlasterCharacter) return;
    BlasterCharacter->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
    if (!EquippedWeapon) return;
    bCanFire = true;
    if (bFireButtonPressed && EquippedWeapon->bAutomatic)
    {
        Fire();
    }
    if (EquippedWeapon->IsEmpty())
    {
        Reload();
    }
}

bool UCombatComponent::CanFire()
{
    if (!EquippedWeapon) return false;
    return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
    CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingARAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, StartingRocketAmmo);
}

void UCombatComponent::SetCarriedAmmo()
{
    if (!HasEquippedWeaponKey()) return;
    CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
}

void UCombatComponent::OnRep_CarriedAmmo()
{
    BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
}

bool UCombatComponent::HasEquippedWeaponKey()
{
    if (!EquippedWeapon) return false;
    return CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType());
}

bool UCombatComponent::IsControllerValid()
{
    return BlasterUtils::CastOrUseExistsActor<ABlasterPlayerController>(BlasterController, BlasterCharacter->GetController());
}