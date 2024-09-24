// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Projectile.h"
#include "ProjectileRocket.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class USoundBase;
class UAudioComponent;
class USoundAttenuation;
class URocketMovementComponent;

UCLASS() class BLASTER_API AProjectileRocket : public AProjectile
{
    GENERATED_BODY()

public:
    AProjectileRocket();

protected:
    virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse,
        const FHitResult& Hit) override;

    virtual void BeginPlay() override;

    void DestroyTimerFinished();

    UPROPERTY(EditAnywhere)
    UNiagaraSystem* TrailSystem;

    UPROPERTY()
    UNiagaraComponent* TrailSystemComponent;

    UPROPERTY()
    UAudioComponent* ProjectileLoopComponent;

    UPROPERTY(EditAnywhere)
    USoundAttenuation* LoopingSoundAttenuation;

    UPROPERTY(EditAnywhere)
    USoundBase* ProjectileLoop;

    UPROPERTY(VisibleAnywhere)
    URocketMovementComponent* RocketMovementComponent;

private:
    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* RocketMesh;

    UPROPERTY(EditAnywhere)
    float DamageInnerRadius = 200.f;

    UPROPERTY(EditAnywhere)
    float DamageOuterRadius = 500.f;

    FTimerHandle DestroyTimer;

    UPROPERTY(EditAnywhere)
    float DestroyTime = 3.f;
};
