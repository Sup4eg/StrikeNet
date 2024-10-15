// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class USoundBase;
class UAudioComponent;
class USoundAttenuation;
class URocketMovementComponent;

UCLASS() class BLASTER_API AProjectileRocket : public AProjectile
{
    GENERATED_BODY()

public:
    AProjectileRocket();

#if WITH_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit) override;

    virtual void BeginPlay() override;

    UPROPERTY()
    UAudioComponent* ProjectileLoopComponent;

    UPROPERTY(EditAnywhere)
    USoundAttenuation* LoopingSoundAttenuation;

    UPROPERTY(EditAnywhere)
    USoundBase* ProjectileLoop;

    UPROPERTY(VisibleAnywhere)
    URocketMovementComponent* RocketMovementComponent;
};
