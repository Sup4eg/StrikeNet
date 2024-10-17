// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInstance.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "Weapon.h"
#include "BuffComp.h"

UBuffComp::UBuffComp()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComp::BeginPlay()
{
    Super::BeginPlay();
}

void UBuffComp::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed, float AimWalkSpeed)
{
    InitialBaseSpeed = BaseSpeed;
    InitialCrouchSpeed = CrouchSpeed;
    InitialAimWalkSpeed = AimWalkSpeed;
}

void UBuffComp::SetInitialJumpVelocity(float Velocity)
{
    InitialJumpSpeed = Velocity;
}

void UBuffComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    HealRampUp(DeltaTime);
    ShieldRampUp(DeltaTime);
}

void UBuffComp::Heal(float HealAmount, float HealingTime)
{
    bHealing = true;
    HealingRate = HealAmount / HealingTime;
    AmountToHeal += HealAmount;
}

void UBuffComp::HealRampUp(float DeltaTime)
{
    if (!bHealing || !BlasterCharacter || BlasterCharacter->GetIsElimmed()) return;

    const float HealThisFrame = HealingRate * DeltaTime;
    BlasterCharacter->SetHealth(FMath::Clamp(BlasterCharacter->GetHealth() + HealThisFrame, 0.f, BlasterCharacter->GetMaxHealth()));
    AmountToHeal -= HealThisFrame;
    BlasterCharacter->UpdateHUDHealth();

    if (AmountToHeal <= 0.f || BlasterCharacter->GetHealth() >= BlasterCharacter->GetMaxHealth())
    {
        bHealing = false;
        AmountToHeal = 0.f;
    }
}

void UBuffComp::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
    bReplenishingShield = true;
    ShieldReplenishRate = ShieldAmount / ReplenishTime;
    ShieldReplenishAmount += ShieldAmount;
}

void UBuffComp::ShieldRampUp(float DeltaTime)
{
    if (!bReplenishingShield || !BlasterCharacter || BlasterCharacter->GetIsElimmed()) return;

    const float ReplenishShieldThisFrame = ShieldReplenishRate * DeltaTime;
    BlasterCharacter->SetShield(
        FMath::Clamp(BlasterCharacter->GetShield() + ReplenishShieldThisFrame, 0.f, BlasterCharacter->GetMaxShield()));
    ShieldReplenishAmount -= ReplenishShieldThisFrame;
    BlasterCharacter->UpdateHUDShield();

    if (ShieldReplenishAmount <= 0.f || BlasterCharacter->GetShield() >= BlasterCharacter->GetMaxShield())
    {
        bReplenishingShield = false;
        ShieldReplenishAmount = 0.f;
    }
}

void UBuffComp::BuffSpeed(float BuffSpeedScaleFactor, float BuffTime)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    BlasterCharacter->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &ThisClass::ResetSpeeds, BuffTime);

    MulticastSpeedBuff(InitialBaseSpeed * BuffSpeedScaleFactor,  //
        InitialCrouchSpeed * BuffSpeedScaleFactor,               //
        InitialAimWalkSpeed * BuffSpeedScaleFactor);
}

void UBuffComp::BuffJump(float BuffJumpScaleFactor, float BuffTime)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    BlasterCharacter->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &ThisClass::ResetJump, BuffTime);
    MulticastJumpBuff(InitialJumpSpeed * BuffJumpScaleFactor);
}

void UBuffComp::BuffInvisibility(float Opacity, float BuffTime)
{
    if (!BlasterCharacter) return;
    BlasterCharacter->GetWorldTimerManager().ClearTimer(InvisibilityBuffTimer);
    BlasterCharacter->GetWorldTimerManager().SetTimer(InvisibilityBuffTimer, this, &ThisClass::ResetInvisibility, BuffTime);

    MulticastStartInvisibilityBuff(Opacity);
}

void UBuffComp::ResetInvisibility()
{
    if (!BlasterCharacter) return;
    MulticastFinishInvisibilityBuff();
}

void UBuffComp::MulticastStartInvisibilityBuff_Implementation(double Opacity)
{
    TargetOpacity = Opacity;
    bIsInvisibility = true;
    SetDynamicInvisibilityMaterialInstance(-0.55f, 200.f);
    PlayInvisibilitySound();
    StartInvisibilityEffect();
}

void UBuffComp::MulticastFinishInvisibilityBuff_Implementation()
{
    PlayInvisibilitySound();
    FinishInvisibilityEffect();
}

void UBuffComp::PlayInvisibilitySound()
{
    if (BlasterCharacter && InvisibilityBuffSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, InvisibilityBuffSound, BlasterCharacter->GetActorLocation());
    }
}

void UBuffComp::StartInvisibilityEffect()
{
    InvisibilityTrack.BindDynamic(this, &ThisClass::UpdateInvisibilityMaterial);
    if (BlasterCharacter && InvisibilityCurve && BlasterCharacter->GetInvisibilityTimeLine())
    {
        BlasterCharacter->GetInvisibilityTimeLine()->AddInterpFloat(InvisibilityCurve, InvisibilityTrack);
        BlasterCharacter->GetInvisibilityTimeLine()->PlayFromStart();
    }
}

void UBuffComp::FinishInvisibilityEffect()
{
    if (BlasterCharacter && BlasterCharacter->GetInvisibilityTimeLine())
    {
        FOnTimelineEvent OnTimelineFinishedCallback;
        OnTimelineFinishedCallback.BindUFunction(this, FName("OnTimelineFinishInvisibilityEffect"));
        BlasterCharacter->GetInvisibilityTimeLine()->SetTimelineFinishedFunc(OnTimelineFinishedCallback);
        BlasterCharacter->GetInvisibilityTimeLine()->ReverseFromEnd();
    }
}

void UBuffComp::OnTimelineFinishInvisibilityEffect()
{
    // Return Init material
    bIsInvisibility = false;
    if (BlasterCharacter && BlasterCharacter->GetInvisibilityTimeLine())
    {
        BlasterCharacter->GetInvisibilityTimeLine()->SetTimelineFinishedFunc(FOnTimelineEvent());
        if (BlasterCharacter->GetMesh())
        {
            BlasterCharacter->SetDefaultMaterial();
        }

        ResetEquippedWeaponInitializedMaterial();
        ResetSecondaryWeaponInitializedMaterial();
    }
}

void UBuffComp::ResetEquippedWeaponInitializedMaterial()
{
    if (BlasterCharacter->IsWeaponEquipped())
    {
        BlasterCharacter->GetEquippedWeapon()->SetDefaultMaterial();
        BlasterCharacter->GetEquippedWeapon()->bIsInvisible = false;
    }
}

void UBuffComp::ResetSecondaryWeaponInitializedMaterial()
{
    if (BlasterCharacter->IsSecondaryWeapon())
    {
        BlasterCharacter->GetSecondaryWeapon()->SetDefaultMaterial();
        BlasterCharacter->GetSecondaryWeapon()->bIsInvisible = false;
    }
}

void UBuffComp::UpdateInvisibilityMaterial(float DissolveValue)
{
    FVector2D OpacityRange(1.f, TargetOpacity);
    float Opacity = FMath::GetMappedRangeValueClamped(DissolveRange, OpacityRange, DissolveValue);
    if (DynamicInvisibilityCharacterMaterialInstance)
    {
        DynamicInvisibilityCharacterMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
        DynamicInvisibilityCharacterMaterialInstance->SetScalarParameterValue(TEXT("Opacity"), Opacity);
    }

    if (DynamicInvisibilityWeaponMaterialInstance)
    {
        DynamicInvisibilityWeaponMaterialInstance->SetScalarParameterValue(TEXT("Opacity"), Opacity);
    }
}

void UBuffComp::SetDynamicInvisibilityMaterialInstance(float Dissolve, float Glow)
{
    // Start Invisibility effect
    if (InvisibilityCharacterMaterialInstance && BlasterCharacter && BlasterCharacter->GetMesh())
    {
        DynamicInvisibilityCharacterMaterialInstance = UMaterialInstanceDynamic::Create(InvisibilityCharacterMaterialInstance, this);
        BlasterCharacter->SetMaterial(DynamicInvisibilityCharacterMaterialInstance);
        DynamicInvisibilityCharacterMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), Dissolve);
        DynamicInvisibilityCharacterMaterialInstance->SetScalarParameterValue(TEXT("Glow"), Glow);
        DynamicInvisibilityCharacterMaterialInstance->SetScalarParameterValue(TEXT("Opacity"), 1.f);
    }
    if (InvisibilityWeaponMaterialInstance)
    {
        DynamicInvisibilityWeaponMaterialInstance = UMaterialInstanceDynamic::Create(InvisibilityWeaponMaterialInstance, this);
    }
    SetDynamicMaterialToEquippedWeapon();
    SetDynamicMaterialToSecondaryWeapon();
}

void UBuffComp::SetDynamicMaterialToEquippedWeapon()
{
    if (InvisibilityWeaponMaterialInstance &&  //
        BlasterCharacter &&                    //
        BlasterCharacter->IsWeaponEquipped())
    {
        BlasterCharacter->GetEquippedWeapon()->SetMaterial(DynamicInvisibilityWeaponMaterialInstance);
        BlasterCharacter->GetEquippedWeapon()->bIsInvisible = true;
    }
}

void UBuffComp::SetDynamicMaterialToSecondaryWeapon()
{
    if (InvisibilityWeaponMaterialInstance &&  //
        BlasterCharacter &&                    //
        BlasterCharacter->IsSecondaryWeapon())
    {
        BlasterCharacter->GetSecondaryWeapon()->SetMaterial(DynamicInvisibilityWeaponMaterialInstance);
        BlasterCharacter->GetSecondaryWeapon()->bIsInvisible = true;
    }
}

void UBuffComp::ResetSpeeds()
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed, InitialAimWalkSpeed);
}

void UBuffComp::ResetJump()
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    MulticastJumpBuff(InitialJumpSpeed);
}

void UBuffComp::MulticastJumpBuff_Implementation(double JumpSpeed)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    BlasterCharacter->GetCharacterMovement()->JumpZVelocity = JumpSpeed;
}

void UBuffComp::MulticastSpeedBuff_Implementation(double BaseSpeed, double CrouchSpeed, double AimSpeed)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;

    BlasterCharacter->BaseWalkSpeed = BaseSpeed;
    BlasterCharacter->CrouchWalkSpeed = CrouchSpeed;
    BlasterCharacter->AimWalkSpeed = AimSpeed;

    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BlasterCharacter->IsAiming() ? AimSpeed : BaseSpeed;
    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}
