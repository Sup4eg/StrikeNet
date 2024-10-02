// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

class UStaticMeshComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UBoxComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class UPrimitiveComponent;
class USoundBase;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();
    virtual void Tick(float DeltaTime) override;

protected:
    virtual void BeginPlay() override;

    void StartDestoryTimer();

    virtual void DestroyTimerFinished();

    void SpawnTrailSystem();

    void ExplodeDamage();

    void PlayFXAndSound(AActor* OtherActor);

    UFUNCTION()
    virtual void OnHit(
        UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHit(AActor* OtherActor);

    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

    UPROPERTY(EditAnywhere)
    UParticleSystem* ImpactParticles;

    UPROPERTY(EditAnywhere)
    UParticleSystem* ImpactCharacterParticles;

    UPROPERTY(EditAnywhere)
    USoundBase* ImpactSound;

    UPROPERTY(EditAnywhere)
    USoundBase* ImpactCharacterSound;

    UPROPERTY(EditAnywhere)
    UBoxComponent* CollisionBox;

    UPROPERTY(EditAnywhere)
    UNiagaraSystem* TrailSystem;

    UPROPERTY()
    UNiagaraComponent* TrailSystemComponent;

    UPROPERTY(VisibleAnywhere)
    UStaticMeshComponent* ProjectileMesh;

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovementComponent;

    UPROPERTY(EditAnywhere)
    float DamageInnerRadius = 200.f;

    UPROPERTY(EditAnywhere)
    float DamageOuterRadius = 500.f;

private:
    UPROPERTY(EditAnywhere)
    UParticleSystem* Tracer;

    UPROPERTY()
    UParticleSystemComponent* TracerComponent;

    FTimerHandle DestroyTimer;

    UPROPERTY(EditAnywhere)
    float DestroyTime = 3.f;

    UPROPERTY(EditDefaultsOnly)
    float LifeSpan = 15.f;

public:
    FORCEINLINE UBoxComponent* GetCollisionBox() const { return CollisionBox; };
};
