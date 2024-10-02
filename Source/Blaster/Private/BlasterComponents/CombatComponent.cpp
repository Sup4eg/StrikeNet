// Fill out your copyright notice in the Description page of Project Settings.

#include "Engine/SkeletalMeshSocket.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
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
#include "Projectile.h"
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
    DOREPLIFETIME(UCombatComponent, Grenades);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
    if (!BlasterCharacter || !WeaponToEquip) return;
    BlasterCharacter->StopAllMontages();
    CombatState = ECombatState::ECS_Unoccupied;

    HandleWeaponSpecificLogic(EquippedWeapon, WeaponToEquip);
    DropEquippedWeapon();
    EquippedWeapon = WeaponToEquip;
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equiped);

    AttachWeaponToRightHand(EquippedWeapon);
    PlayEquipWeaponSound();
    BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
    BlasterCharacter->bUseControllerRotationYaw = true;

    EquippedWeapon->SetOwner(BlasterCharacter);
    EquippedWeapon->SetHUDAmmo();

    SetCarriedAmmo();

    // Set HUD Weapon info
    if (!IsControllerValid()) return;
    BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
    BlasterController->SetHUDWeaponIcon(EquippedWeapon->WeaponIcon);
    BlasterController->ShowHUDWeaponInfo();
    BlasterController->ShowHUDGrenadeInfo();
    BlasterController->SetHUDGrenadesAmount(Grenades);

    ReloadEmptyWeapon();
}

void UCombatComponent::OnRep_EquippedWeapon(AWeapon* LastEquippedWeapon)
{
    if (!EquippedWeapon || !BlasterCharacter) return;
    BlasterCharacter->StopAllMontages();
    HandleWeaponSpecificLogic(LastEquippedWeapon, EquippedWeapon);

    AttachWeaponToRightHand(EquippedWeapon);
    PlayEquipWeaponSound();
    BlasterCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
    BlasterCharacter->bUseControllerRotationYaw = true;

    // Set HUD Weapon info
    if (!IsControllerValid()) return;
    BlasterController->SetHUDWeaponIcon(EquippedWeapon->WeaponIcon);
    BlasterController->ShowHUDWeaponInfo();
    BlasterController->ShowHUDGrenadeInfo();
    BlasterController->SetHUDGrenadesAmount(Grenades);
}

void UCombatComponent::DropEquippedWeapon()
{
    if (EquippedWeapon)
    {
        EquippedWeapon->Dropped();
    }
}

void UCombatComponent::AttachWeaponToRightHand(AWeapon* WeaponToAttach)
{
    if (!BlasterCharacter || !BlasterCharacter->GetMesh() || !WeaponToAttach) return;
    const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName("RightHandSocket");
    if (HandSocket && WeaponToAttach->GetWeaponMesh())
    {
        WeaponToAttach->GetWeaponMesh()->SetSimulatePhysics(false);
        HandSocket->AttachActor(WeaponToAttach, BlasterCharacter->GetMesh());
    }
}

void UCombatComponent::AttachWeaponToLeftHand(AWeapon* WeaponToAttach)
{
    if (!BlasterCharacter || !BlasterCharacter->GetMesh() || !WeaponToAttach) return;
    bool bUsePistolSocket = (WeaponToAttach->GetWeaponType() == EWeaponType::EWT_Pistol ||  //
                             WeaponToAttach->GetWeaponType() == EWeaponType::EWT_SMG);
    FName SocketName = bUsePistolSocket ? FName("LeftHandPistolSocket") : FName("LeftHandSocket");
    const USkeletalMeshSocket* HandSocket = BlasterCharacter->GetMesh()->GetSocketByName(SocketName);
    if (HandSocket && WeaponToAttach->GetWeaponMesh())
    {
        WeaponToAttach->GetWeaponMesh()->SetSimulatePhysics(false);
        HandSocket->AttachActor(WeaponToAttach, BlasterCharacter->GetMesh());
    }
}

void UCombatComponent::ReloadEmptyWeapon()
{
    if (EquippedWeapon && EquippedWeapon->IsEmpty())
    {
        Reload();
    }
}

void UCombatComponent::PlayEquipWeaponSound()
{
    if (BlasterCharacter && EquippedWeapon && EquippedWeapon->EquipSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, EquippedWeapon->EquipSound, BlasterCharacter->GetActorLocation());
    }
}

void UCombatComponent::HandleWeaponSpecificLogic(AWeapon* LastWeapon, AWeapon* NewWeapon)
{
    if (!LastWeapon || !NewWeapon || !BlasterCharacter->IsLocallyControlled()) return;

    if (LastWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle && LastWeapon->GetWeaponType() != NewWeapon->GetWeaponType())
    {
        BlasterCharacter->bDrawCrosshair = true;
        if (bAiming)
        {
            BlasterCharacter->ShowSniperScopeWidget(false);
            if (IsControllerValid())
            {
                BlasterController->ShowHUDCharacterOverlay();
            }
        }
    }
    else if (NewWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
    {
        if (bAiming)
        {
            BlasterCharacter->ShowSniperScopeWidget(true);
            BlasterCharacter->bDrawCrosshair = false;
            if (IsControllerValid())
            {
                BlasterController->HideHUDCharacterOverlay();
            }
        }
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

void UCombatComponent::UpdateShotgunAmmoValues()
{
    if (!BlasterCharacter || !EquippedWeapon || !HasEquippedWeaponKey() || EquippedWeapon->GetWeaponType() != EWeaponType::EWT_Shotgun)
        return;
    CarriedAmmoMap[EquippedWeapon->GetWeaponType()] -= 1;
    CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
    if (IsControllerValid())
    {
        BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
    }
    EquippedWeapon->AddAmmo(-1);
    bCanFire = true;
    if (EquippedWeapon->IsFull() || CarriedAmmo == 0)
    {
        BlasterCharacter->PlayMontage(BlasterCharacter->GetReloadMontage(), FName("ShotgunEnd"));
    }
}

bool UCombatComponent::CanReload()
{
    return EquippedWeapon &&                                                //
           EquippedWeapon->GetAmmo() < EquippedWeapon->GetMagCapacity() &&  //
           CarriedAmmo > 0 &&                                               //
           CombatState == ECombatState::ECS_Unoccupied;                     //
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
        case ECombatState::ECS_ThrowingGrenade:
            if (BlasterCharacter && !BlasterCharacter->IsLocallyControlled())
            {
                AttachWeaponToLeftHand(EquippedWeapon);
                BlasterCharacter->PlayThrowGrenadeMontage();
                ShowAttachedGrenade(true);
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
    if (!EquippedWeapon || !BlasterCharacter) return;
    bAiming = bIsAiming;
    ServerSetAiming(bIsAiming);
    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    if (BlasterCharacter->IsLocallyControlled() && EquippedWeapon->GetWeaponType() == EWeaponType::EWT_SniperRifle)
    {
        BlasterCharacter->ShowSniperScopeWidget(bIsAiming);
        BlasterCharacter->bDrawCrosshair = !bIsAiming;
        if (IsControllerValid())
        {
            if (bAiming)
            {
                BlasterController->HideHUDCharacterOverlay();
            }
            else
            {
                BlasterController->ShowHUDCharacterOverlay();
            }
        }
    }
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
    if (!EquippedWeapon || !BlasterCharacter) return;
    bAiming = bIsAiming;
    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
    bFireButtonPressed = bPressed;
    if (bFireButtonPressed)
    {
        Fire();
    }
}

void UCombatComponent::ShotgunShellReload()
{
    if (BlasterCharacter && BlasterCharacter->HasAuthority())
    {
        UpdateShotgunAmmoValues();
    }
}

void UCombatComponent::ThrowGrenadeFinished()
{
    CombatState = ECombatState::ECS_Unoccupied;
    AttachWeaponToRightHand(EquippedWeapon);
}

void UCombatComponent::LaunchGrenade()
{
    ShowAttachedGrenade(false);
    if (BlasterCharacter && BlasterCharacter->IsLocallyControlled())
    {
        ServerLaunchGranade(HitTarget);
    }
}

void UCombatComponent::ServerLaunchGranade_Implementation(const FVector_NetQuantize Target)
{
    bool bSpawnGrenade = BlasterCharacter &&                        //
                         GrenadeClass &&                            //
                         BlasterCharacter->GetAttachedGrenade() &&  //
                         GetWorld();                                //

    if (bSpawnGrenade)
    {
        const FVector StartingLocation = BlasterCharacter->GetAttachedGrenade()->GetComponentLocation();
        FVector ToTarget = Target - StartingLocation;
        FActorSpawnParameters SpawnParams;
        SpawnParams.Owner = BlasterCharacter;
        SpawnParams.Instigator = BlasterCharacter;

        AProjectile* Grenade = GetWorld()->SpawnActor<AProjectile>(  //
            GrenadeClass,                                            //
            StartingLocation,                                        //
            ToTarget.Rotation(),                                     //
            SpawnParams);

        if (Grenade)
        {
            FCollisionQueryParams QueryParams;
            QueryParams.AddIgnoredActor(SpawnParams.Owner);
            Grenade->GetCollisionBox()->IgnoreActorWhenMoving(SpawnParams.Owner, true);
        }
    }
}

void UCombatComponent::PickupAmmo(EWeaponType WeaponType, uint32 AmmoAmount)
{
    if (CarriedAmmoMap.Contains(WeaponType) && MaxAmmoMap.Contains(WeaponType))
    {
        CarriedAmmoMap[WeaponType] = FMath::Clamp(CarriedAmmoMap[WeaponType] + AmmoAmount, 0, MaxAmmoMap[WeaponType]);
        SetCarriedAmmo();
        if (IsControllerValid())
        {
            BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
        }
    }

    if (EquippedWeapon && EquippedWeapon->GetWeaponType() == WeaponType)
    {
        Reload();
    }
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
    MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
    if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::ECS_Reloading &&
        EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
    {
        BlasterCharacter->PlayFireMontage(bAiming);
        EquippedWeapon->Fire(TraceHitTarget);
        CombatState = ECombatState::ECS_Unoccupied;
        return;
    }
    if (BlasterCharacter && EquippedWeapon && CombatState == ECombatState::ECS_Unoccupied)
    {
        BlasterCharacter->PlayFireMontage(bAiming);
        EquippedWeapon->Fire(TraceHitTarget);
    }
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
        if (!BlasterCharacter || BlasterCharacter->GetIsGameplayDisabled() || !BlasterCharacter->bDrawCrosshair || !EquippedWeapon)
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

void UCombatComponent::ThrowGrenade()
{
    if (Grenades == 0 || CombatState != ECombatState::ECS_Unoccupied || !EquippedWeapon) return;
    CombatState = ECombatState::ECS_ThrowingGrenade;
    if (BlasterCharacter)
    {
        BlasterCharacter->PlayThrowGrenadeMontage();
        AttachWeaponToLeftHand(EquippedWeapon);
        ShowAttachedGrenade(true);
    }
    if (BlasterCharacter && !BlasterCharacter->HasAuthority())
    {
        ServerThrowGrenade();
    }
    if (BlasterCharacter && BlasterCharacter->HasAuthority())
    {
        Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
        UpdateHUDGrenades();
    }
}

void UCombatComponent::ServerThrowGrenade_Implementation()
{
    if (Grenades == 0) return;
    CombatState = ECombatState::ECS_ThrowingGrenade;
    if (BlasterCharacter)
    {
        BlasterCharacter->PlayThrowGrenadeMontage();
        AttachWeaponToLeftHand(EquippedWeapon);
        ShowAttachedGrenade(true);
    }
    Grenades = FMath::Clamp(Grenades - 1, 0, MaxGrenades);
    UpdateHUDGrenades();
}

void UCombatComponent::UpdateHUDGrenades()
{
    if (IsControllerValid())
    {
        BlasterController->SetHUDGrenadesAmount(Grenades);
    }
}

void UCombatComponent::ShowAttachedGrenade(bool bShowAttachedGrenade)
{
    if (!BlasterCharacter || !BlasterCharacter->GetAttachedGrenade()) return;
    BlasterCharacter->GetAttachedGrenade()->SetVisibility(bShowAttachedGrenade);
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
    ReloadEmptyWeapon();
}

bool UCombatComponent::CanFire()
{
    if (!EquippedWeapon) return false;
    if (!EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Reloading &&
        EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun)
    {
        return true;
    }
    return !EquippedWeapon->IsEmpty() && bCanFire && CombatState == ECombatState::ECS_Unoccupied;
}

void UCombatComponent::InitializeCarriedAmmo()
{
    CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, MaxARAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, MaxRocketAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_Pistol, MaxPistolAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_SMG, MaxSMGAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_Shotgun, MaxShotgunAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, MaxSniperRifleAmmo);
    CarriedAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, MaxGrenadeLauncherAmmo);

    MaxAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, MaxARAmmo);
    MaxAmmoMap.Emplace(EWeaponType::EWT_RocketLauncher, MaxRocketAmmo);
    MaxAmmoMap.Emplace(EWeaponType::EWT_Pistol, MaxPistolAmmo);
    MaxAmmoMap.Emplace(EWeaponType::EWT_SMG, MaxSMGAmmo);
    MaxAmmoMap.Emplace(EWeaponType::EWT_Shotgun, MaxShotgunAmmo);
    MaxAmmoMap.Emplace(EWeaponType::EWT_SniperRifle, MaxSniperRifleAmmo);
    MaxAmmoMap.Emplace(EWeaponType::EWT_GrenadeLauncher, MaxGrenadeLauncherAmmo);
}

void UCombatComponent::SetCarriedAmmo()
{
    if (!HasEquippedWeaponKey()) return;
    CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
}

void UCombatComponent::OnRep_CarriedAmmo()
{
    bool bJumpToShotgunEnd = EquippedWeapon &&                                               //
                             EquippedWeapon->GetWeaponType() == EWeaponType::EWT_Shotgun &&  //
                             CombatState == ECombatState::ECS_Reloading &&                   //
                             BlasterCharacter &&                                             //
                             CarriedAmmo == 0;
    if (bJumpToShotgunEnd)
    {
        BlasterCharacter->PlayMontage(BlasterCharacter->GetReloadMontage(), FName("ShotgunEnd"));
    }

    if (IsControllerValid())
    {
        BlasterController->SetHUDCarriedAmmo(CarriedAmmo);
    }
}

void UCombatComponent::OnRep_Grenades()
{
    UpdateHUDGrenades();
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