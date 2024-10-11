// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileGrenade.generated.h"

class USoundBase;

UCLASS()
class BLASTER_API AProjectileGrenade : public AProjectile
{
    GENERATED_BODY()

public:
    AProjectileGrenade();

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

    virtual void DestroyTimerFinished() override;

private:
    FHitResult GetClosestResultToExplosion();

    UPROPERTY(EditAnywhere)
    USoundBase* BounceSound;

    UPROPERTY(EditAnywhere)
    float TraceDecalRadius = 100.f;
};
