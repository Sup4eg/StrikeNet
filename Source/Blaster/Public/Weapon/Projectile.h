// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WeaponTypes.h"
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
class UPhysicalMaterial;

UCLASS()
class BLASTER_API AProjectile : public AActor
{
    GENERATED_BODY()

public:
    AProjectile();
    virtual void Tick(float DeltaTime) override;

    /**
     * Used with Server Side Rewind
     */
    bool bUseServerSideRewind = false;
    FVector_NetQuantize TraceStart;
    FVector_NetQuantize100 InitialVelocity;

    UPROPERTY(EditAnywhere, Category = "Movement")
    float InitialSpeed = 15000.f;
protected:
    virtual void BeginPlay() override;

    void StartDestoryTimer();

    virtual void DestroyTimerFinished();

    void SpawnTrailSystem();

    void ExplodeDamage();

    virtual void SpawnImpactFXAndSound(const FHitResult& Hit);
    virtual void SpawnImpactParticles(const FHitResult& FireHit, FImpactData& ImpactData);
    virtual void SpawnImpactSound(const FHitResult& FireHit, FImpactData& ImpactData);
    virtual void SpawnImpactDecal(const FHitResult& FireHit, FImpactData& ImpactData);

    UFUNCTION()
    virtual void OnHit(
        UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

    UFUNCTION(NetMulticast, Reliable)
    void MulticastHit(const FHitResult& Hit);

    UPROPERTY(EditAnywhere)
    float Damage = 20.f;

    UPROPERTY(EditAnywhere)
    FImpactData DefaultImpactData;

    UPROPERTY(EditAnywhere)
    TMap<UPhysicalMaterial*, FImpactData> ImpactDataMap;

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
    FImpactData GetImpactData(const FHitResult& FireHit);

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
