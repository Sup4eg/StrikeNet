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

    if (Character && Character->IsLocallyControlled())
    {
        FHitResult HitResult;
        TraceUnderCrosshairs(HitResult);
        HitTarget = HitResult.ImpactPoint;

        SetHUDCrosshairs(DeltaTime);
        InterpFOV(DeltaTime);
    }
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UCombatComponent, EquippedWeapon);
    DOREPLIFETIME(UCombatComponent, bAiming);
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
    if (!Character || !WeaponToEquip) return;
    EquippedWeapon = WeaponToEquip;
    EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equiped);
    const USkeletalMeshSocket* HandSocket = Character->GetMesh()->GetSocketByName("RightHandSocket");
    if (HandSocket)
    {
        HandSocket->AttachActor(EquippedWeapon, Character->GetMesh());
    }
    EquippedWeapon->SetOwner(Character);
    Character->GetCharacterMovement()->bOrientRotationToMovement = false;
    Character->bUseControllerRotationYaw = true;
}

void UCombatComponent::BeginPlay()
{
    Super::BeginPlay();
    if (!Character || !Character->GetFollowCamera()) return;
    check(Character->GetCharacterMovement());
    Character->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

    DefaultFOV = Character->GetFollowCamera()->FieldOfView;
    CurrentFOV = DefaultFOV;
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
    bAiming = bIsAiming;
    ServerSetAiming(bIsAiming);
    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
    bAiming = bIsAiming;
    if (Character)
    {
        Character->GetCharacterMovement()->MaxWalkSpeed = bIsAiming ? AimWalkSpeed : BaseWalkSpeed;
    }
}

void UCombatComponent::OnRep_EquippedWeapon()
{
    if (EquippedWeapon && Character)
    {
        Character->GetCharacterMovement()->bOrientRotationToMovement = false;
        Character->bUseControllerRotationYaw = true;
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
    if (!Character || !EquippedWeapon) return;
    Character->PlayFireMontage(bAiming);
    EquippedWeapon->Fire(TraceHitTarget);
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
    if (!Character || !GEngine || !GEngine->GameViewport) return;
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
        float DistanceToCharacter = (Character->GetActorLocation() - Start).Size();
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
    if (!Character || !Character->Controller || !EquippedWeapon) return;
    Controller = Controller == nullptr ? Cast<ABlasterPlayerController>(Character->GetController()) : Controller;
    if (Controller)
    {
        HUD = HUD == nullptr ? Cast<ABlasterHUD>(Controller->GetHUD()) : HUD;
        if (HUD)
        {
            HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
            HUDPackage.CrosshairsLeft = EquippedWeapon->CrosshairsLeft;
            HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
            HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
            HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
            HUDPackage.CrosshairSpread = GetCrosshairsSpread(DeltaTime);
            HUD->SetHUDPackage(HUDPackage);
        }
    }
}

float UCombatComponent::GetCrosshairsSpread(float DeltaTime)
{
    // Calculate crosshair spread
    //[0, 600] -> [0, 1]
    FVector2D WalkSpeedRange(0.f,
        Character->bIsCrouched ? Character->GetCharacterMovement()->MaxWalkSpeedCrouched : Character->GetCharacterMovement()->MaxWalkSpeed);

    FVector2D VelocityMultiplayerRange(0.f, Character->bIsCrouched ? 0.5f : 1.f);
    FVector Velocity = Character->GetVelocity();
    Velocity.Z = 0.f;

    CrosshairVelocityFactor = FMath::GetMappedRangeValueClamped(WalkSpeedRange, VelocityMultiplayerRange, Velocity.Size());
    if (Character->IsInAir())
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
    if (!EquippedWeapon || !Character || !Character->GetFollowCamera()) return;
    if (bAiming)
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaTime, EquippedWeapon->GetZoomInterpSpeed());
    }
    else
    {
        CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaTime, ZoomInterpSpeed);
    }
    Character->GetFollowCamera()->SetFieldOfView(CurrentFOV);
}

void UCombatComponent::Fire()
{
    if (bCanFire)
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
    if (!EquippedWeapon || !Character) return;
    Character->GetWorldTimerManager().SetTimer(FireTimer, this, &UCombatComponent::FireTimerFinished, EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
    if (!EquippedWeapon) return;
    bCanFire = true;
    if (bFireButtonPressed && EquippedWeapon->bAutomatic)
    {
        Fire();
    }
}
