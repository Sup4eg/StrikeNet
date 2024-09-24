// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"

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

    UPROPERTY(VisibleAnywhere)
    UProjectileMovementComponent* ProjectileMovementComponent;
    
    void PlayFXAndSound(AActor* OtherActor);


private:
    UPROPERTY(EditAnywhere)
    UParticleSystem* Tracer;

    UPROPERTY()
    UParticleSystemComponent* TracerComponent;
};
