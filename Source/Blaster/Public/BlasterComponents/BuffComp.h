// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Components/TimelineComponent.h"
#include "BuffComp.generated.h"

class ABlasterCharacter;
class UMaterialInstanceDynamic;
class UMaterialInstance;
class UMaterialInterface;
class USoundBase;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class BLASTER_API UBuffComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UBuffComp();

    friend class ABlasterCharacter;

    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

    void Heal(float HealAmount, float HealingTime);
    void ReplenishShield(float ShieldAmount, float ReplenishTime);

    void BuffSpeed(float BuffSpeedScaleFactor, float BuffTime);
    void BuffJump(float BuffJumpScaleFactor, float BuffTime);
    void BuffInvisibility(float Opacity, float BuffTime);

    void SetInitialSpeeds(float BaseSpeed, float CrouchSpeed, float AimSpeed);

    void SetInitialJumpVelocity(float Velocity);

    void ResetEquippedWeaponInitializedMaterial();

    void ResetSecondaryWeaponInitializedMaterial();

    void SetDynamicMaterialToEquippedWeapon();
    void SetDynamicMaterialToSecondaryWeapon();

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

    /**
     * Invisibility Buff
     */
    FTimerHandle InvisibilityBuffTimer;
    void ResetInvisibility();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastStartInvisibilityBuff(double Opacity);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastFinishInvisibilityBuff();

    void StartInvisibilityEffect();

    void FinishInvisibilityEffect();

    UFUNCTION()
    void UpdateInvisibilityMaterial(float DissolveValue);

    UFUNCTION()
    void OnTimelineFinishInvisibilityEffect();

    void SetDynamicInvisibilityMaterialInstance(float Dissolve, float Glow);

    void PlayInvisibilitySound();

    FOnTimelineFloat InvisibilityTrack;

    UPROPERTY(EditAnywhere, Category = "Invisibility Buff")
    UCurveFloat* InvisibilityCurve;

    // dynamic instance that we can change at runtime
    UPROPERTY(VisibleAnywhere, Category = "Invisibility Buff")
    UMaterialInstanceDynamic* DynamicInvisibilityCharacterMaterialInstance;

    // Material instance set on the Blueprint, used with the dynamic material instance
    UPROPERTY(EditAnywhere, Category = "Invisibility Buff")
    UMaterialInstance* InvisibilityCharacterMaterialInstance;

    // dynamic instance that we can change at runtime
    UPROPERTY(VisibleAnywhere, Category = "Invisibility Buff")
    UMaterialInstanceDynamic* DynamicInvisibilityWeaponMaterialInstance;

    // Material instance set on the Blueprint, used with the dynamic material instance
    UPROPERTY(EditAnywhere, Category = "Invisibility Buff")
    UMaterialInstance* InvisibilityWeaponMaterialInstance;

    FVector2D DissolveRange = {-0.55, 0.55};

    float TargetOpacity = 0.05f;

    UPROPERTY(EditAnywhere)
    USoundBase* InvisibilityBuffSound;

    bool bIsInvisibility = false;

public:
    FORCEINLINE bool IsInvisibilityActive() const { return bIsInvisibility; };
};
