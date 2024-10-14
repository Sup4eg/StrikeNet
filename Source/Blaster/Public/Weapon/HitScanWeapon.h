// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Weapon.h"
#include "Weapontypes.h"
#include "HitScanWeapon.generated.h"

class UParticleSystem;
class USoundBase;
class UPhysicalMaterial;

UCLASS()
class BLASTER_API AHitScanWeapon : public AWeapon
{
    GENERATED_BODY()

public:
    virtual void Fire(const FVector& HitTarget) override;

protected:
    void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
    virtual void SpawnImpactFXAndSound(FHitResult& FireHit);
    virtual void SpawnImpactParticles(FHitResult& FireHit, FImpactData& ImpactData);
    virtual void SpawnImpactSound(FHitResult& FireHit, FImpactData& ImpactData);
    virtual void SpawnImpactDecal(FHitResult& FireHit, FImpactData& ImpactData);

    FImpactData GetImpactData(FHitResult& FireHit);

    UPROPERTY(EditAnywhere)
    FImpactData DefaultImpactData;

    UPROPERTY(EditAnywhere)
    TMap<UPhysicalMaterial*, FImpactData> ImpactDataMap;

    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

private:
    UPROPERTY(EditAnywhere)
    UParticleSystem* BeamParticles;

    UPROPERTY(EditAnywhere)
    UParticleSystem* MuzzleFlash;

    UPROPERTY(EditAnywhere)
    USoundBase* FireSound;
};
