// Fill out your copyright notice in the Description page of Project Settings.

#include "BlasterCharacter.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "BuffComponent.h"

UBuffComponent::UBuffComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

void UBuffComponent::BeginPlay()
{
    Super::BeginPlay();
}

void UBuffComponent::SetInitialSpeeds(float BaseSpeed, float CrouchSpeed, float AimWalkSpeed)
{
    InitialBaseSpeed = BaseSpeed;
    InitialCrouchSpeed = CrouchSpeed;
    InitialAimWalkSpeed = AimWalkSpeed;
}

void UBuffComponent::SetInitialJumpVelocity(float Velocity)
{
    InitialJumpSpeed = Velocity;
}

void UBuffComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    HealRampUp(DeltaTime);
    ShieldRampUp(DeltaTime);
}

void UBuffComponent::Heal(float HealAmount, float HealingTime)
{
    bHealing = true;
    HealingRate = HealAmount / HealingTime;
    AmountToHeal += HealAmount;
}

void UBuffComponent::HealRampUp(float DeltaTime)
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

void UBuffComponent::ReplenishShield(float ShieldAmount, float ReplenishTime)
{
    bReplenishingShield = true;
    ShieldReplenishRate = ShieldAmount / ReplenishTime;
    ShieldReplenishAmount += ShieldAmount;
}

void UBuffComponent::ShieldRampUp(float DeltaTime)
{
    if (!bReplenishingShield || !BlasterCharacter || BlasterCharacter->GetIsElimmed()) return;

    const float ReplenishShieldThisFrame = ShieldReplenishRate * DeltaTime;
    BlasterCharacter->SetShield(FMath::Clamp(BlasterCharacter->GetShield() + ReplenishShieldThisFrame, 0.f, BlasterCharacter->GetMaxShield()));
    ShieldReplenishAmount -= ReplenishShieldThisFrame;
    BlasterCharacter->UpdateHUDShield();

    if (ShieldReplenishAmount <= 0.f || BlasterCharacter->GetShield() >= BlasterCharacter->GetMaxShield())
    {
        bReplenishingShield = false;
        ShieldReplenishAmount = 0.f;
    }
}

void UBuffComponent::BuffSpeed(float BuffSpeedScaleFactor, float BuffTime)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    BlasterCharacter->GetWorldTimerManager().SetTimer(SpeedBuffTimer, this, &ThisClass::ResetSpeeds, BuffTime);

    MulticastSpeedBuff(InitialBaseSpeed * BuffSpeedScaleFactor,  //
        InitialCrouchSpeed * BuffSpeedScaleFactor,               //
        InitialAimWalkSpeed * BuffSpeedScaleFactor);
}

void UBuffComponent::BuffJump(float BuffJumpScaleFactor, float BuffTime)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    BlasterCharacter->GetWorldTimerManager().SetTimer(JumpBuffTimer, this, &ThisClass::ResetJump, BuffTime);
    MulticastJumpBuff(InitialJumpSpeed * BuffJumpScaleFactor);
}

void UBuffComponent::ResetSpeeds()
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    MulticastSpeedBuff(InitialBaseSpeed, InitialCrouchSpeed, InitialAimWalkSpeed);
}

void UBuffComponent::ResetJump()
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    MulticastJumpBuff(InitialJumpSpeed);
}

void UBuffComponent::MulticastJumpBuff_Implementation(double JumpSpeed)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;
    BlasterCharacter->GetCharacterMovement()->JumpZVelocity = JumpSpeed;
}

void UBuffComponent::MulticastSpeedBuff_Implementation(double BaseSpeed, double CrouchSpeed, double AimSpeed)
{
    if (!BlasterCharacter || !BlasterCharacter->GetCharacterMovement()) return;

    BlasterCharacter->BaseWalkSpeed = BaseSpeed;
    BlasterCharacter->CrouchWalkSpeed = CrouchSpeed;
    BlasterCharacter->AimWalkSpeed = AimSpeed;

    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeed = BlasterCharacter->IsAiming() ? AimSpeed : BaseSpeed;
    BlasterCharacter->GetCharacterMovement()->MaxWalkSpeedCrouched = CrouchSpeed;
}
