// Fill out your copyright notice in the Description page of Project Settings.

#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectileGrenade.h"

AProjectileGrenade::AProjectileGrenade()
{
    ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>("Grenade Mesh");
    ProjectileMesh->SetupAttachment(RootComponent);
    ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovementComponent");
    ProjectileMovementComponent->bRotationFollowsVelocity = true;
    ProjectileMovementComponent->SetIsReplicated(true);
    ProjectileMovementComponent->bShouldBounce = true;
}

void AProjectileGrenade::BeginPlay()
{
    AActor::BeginPlay();

    SpawnTrailSystem();
    StartDestoryTimer();

    ProjectileMovementComponent->OnProjectileBounce.AddDynamic(this, &ThisClass::OnBounce);
}

void AProjectileGrenade::OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity)
{
    if (BounceSound)
    {
        UGameplayStatics::PlaySoundAtLocation(this, BounceSound, GetActorLocation());
    }
}

void AProjectileGrenade::DestroyTimerFinished()
{
    ExplodeDamage();
    MulticastHit(nullptr);
    Super::DestroyTimerFinished();
}
