// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "BuffComponent.generated.h"

class ABlasterCharacter;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLASTER_API UBuffComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UBuffComponent();
    friend class ABlasterCharacter;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void Heal(float HealAmount, float HealingTime);
    void ReplenishShield(float ShieldAmount, float ReplenishTime);

    void BuffSpeed(float BuffSpeedScaleFactor, float BuffTime);
    void BuffJump(float BuffJumpScaleFactor, float BuffTime);

    void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed, float AimSpeed);

    void SetInitialJumpVelocity(float Velocity);

protected:
    virtual void BeginPlay() override;

    void HealRampUp(float DeltaTime);
    void ShieldRampUp(float DeltaTime);

private:
    UPROPERTY()
    ABlasterCharacter* BlasterCharacter;

    /**
     * Heal Buff
     */

    bool bHealing = false;
    float HealingRate = 0.f;
    float AmountToHeal = 0.f;

    /**
     * Shield Buff
     */

    bool bReplenishingShield = false;
    float ShieldReplenishRate = 0.f;
    float ShieldReplenishAmount = 0.f;

    /**
     * Speed Buff
     */
    FTimerHandle SpeedBuffTimer;
    void ResetSpeeds();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSpeedBuff(double BaseSpeed, double CrouchSpeed, double AimSpeed);

    float InitialBaseSpeed;
    float InitialCrouchSpeed;
    float InitialAimWalkSpeed;

    /**
     * Jump buff
     */

    FTimerHandle JumpBuffTimer;
    void ResetJump();
    float InitialJumpSpeed;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastJumpBuff(double JumpSpeed);
};
