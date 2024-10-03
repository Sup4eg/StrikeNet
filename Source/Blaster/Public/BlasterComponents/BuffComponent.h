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

    void BuffSpeed(float BuffSpeedScaleFactor, float BuffTime);

    void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed, float AimSpeed);

protected:
    virtual void BeginPlay() override;

    void HealRampUp(float DeltaTime);

private:
    UPROPERTY()
    ABlasterCharacter* BlasterCharacter;

    /**
     * Heal Buff
     */

    bool bHealing = false;
    float HealingRate = 0;
    float AmountToHeal = 0.f;

    /**
     * Speed Buff
     */
    FTimerHandle SpeedBuffTimer;
    void ResetSpeeds();

    float InitialBaseSpeed;
    float InitialCrouchSpeed;
    float InitialAimWalkSpeed;

    UFUNCTION(NetMulticast, Reliable)
    void MulticastSpeedBuff(float BaseSpeed, float CrouchSpeed, float AimSpeed);
};
